#include "SkeletalMeshComponent.h"

#include "ReferenceSkeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "Misc/FrameTime.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimTypes.h"
#include "Contents/AnimInstance/MyAnimInstance.h"
#include "Engine/Engine.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"
#include "World/World.h"

bool USkeletalMeshComponent::bIsCPUSkinning = false;

USkeletalMeshComponent::USkeletalMeshComponent()
    : AnimationMode(EAnimationMode::AnimationSingleNode)
    , SkeletalMeshAsset(nullptr)
    , AnimClass(nullptr)
    , AnimScriptInstance(nullptr)
    , bPlayAnimation(true)
    ,BonePoseContext(nullptr)
{
    CPURenderData = std::make_unique<FSkeletalMeshRenderData>();
}

void USkeletalMeshComponent::InitializeComponent()
{
    Super::InitializeComponent();

    InitAnim();
}

UObject* USkeletalMeshComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->SetSkeletalMeshAsset(SkeletalMeshAsset);
    NewComponent->SetAnimationMode(AnimationMode);
    if (AnimationMode == EAnimationMode::AnimationBlueprint)
    {
        NewComponent->SetAnimClass(AnimClass);
        UMyAnimInstance* AnimInstance = Cast<UMyAnimInstance>(NewComponent->GetAnimInstance());
        AnimInstance->SetPlaying(Cast<UMyAnimInstance>(AnimScriptInstance)->IsPlaying());
        // TODO: 애님 인스턴스 세팅하기
    }
    else
    {
        NewComponent->SetAnimation(GetAnimation());
    }
    NewComponent->SetLooping(this->IsLooping());
    NewComponent->SetPlaying(this->IsPlaying());
    return NewComponent;
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (GetPhysicsAsset() && !Bodies.IsEmpty())
    {
        UpdatePosePhysics();
    }
    TickPose(DeltaTime);
}

void USkeletalMeshComponent::UpdatePosePhysics()
{
    if (!SkeletalMeshAsset || !SkeletalMeshAsset->GetSkeleton() || Bodies.IsEmpty())
    {
        return;
    }

    const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetRefSkeleton();

    // 임시: 현재 프레임의 애니메이션 기반 글로벌 본 트랜스폼 (월드 공간)
    // 이 배열은 루프 내에서 필요에 따라 채워지거나, 루프 시작 전에 미리 계산될 수 있습니다.
    // 여기서는 각 본의 물리 결과를 적용할 때, 해당 본의 "애니메이션" 부모 트랜스폼을 참조하기 위해 사용합니다.
    TArray<FTransform> TempAnimGlobalBoneTransforms;
    TempAnimGlobalBoneTransforms.SetNum(RefSkeleton.GetRawBoneNum());

    // 1단계: 현재 애니메이션 포즈를 기반으로 글로벌 트랜스폼 계산 (또는 이전 프레임의 최종 포즈)
    // 이 부분은 GetCurrentGlobalBoneMatrices와 유사하게 BonePoseContext.Pose를 사용하여 계산합니다.
    // (정확성을 위해 GetCurrentGlobalBoneMatrices를 직접 호출하거나 해당 로직을 여기에 통합)
    for (int32 BoneIdx = 0; BoneIdx < RefSkeleton.GetRawBoneNum(); ++BoneIdx)
    {
        const FTransform& BoneLocalAnimTransform = BonePoseContext.Pose[BoneIdx]; // 현재 애니메이션 로컬 포즈
        int32 ParentIdx = RefSkeleton.GetParentIndex(BoneIdx);
        if (ParentIdx != INDEX_NONE)
        {
            TempAnimGlobalBoneTransforms[BoneIdx] = BoneLocalAnimTransform * TempAnimGlobalBoneTransforms[ParentIdx];
        }
        else
        {
            TempAnimGlobalBoneTransforms[BoneIdx] = BoneLocalAnimTransform;
        }
        // 스케일은 여기서도 정규화해주는 것이 좋습니다.
        TempAnimGlobalBoneTransforms[BoneIdx].NormalizeRotation();
        TempAnimGlobalBoneTransforms[BoneIdx].SetScale3D(FVector::OneVector); // 애니메이션 포즈의 스케일도 1로 가정
    }


    // 각 FBodyInstance (물리 바디)에 대해 반복
    for (FBodyInstance* Body : Bodies)
    {
        if (!Body || !Body->RigidBody || !Body->GetBodySetup()) // 유효성 검사
        {
            continue;
        }

        // 1. 물리 바디의 현재 월드 트랜스폼 가져오기
        // Body->WorldTransform은 PhysX 시뮬레이션 결과로 이미 업데이트되었다고 가정합니다.
        const FTransform& PhysicsBodyWorldTransform_FromSim = Body->WorldTransform;
        const FName& BoneName = Body->GetBodySetup()->BoneName;
        int32 CurrentBoneIndex = RefSkeleton.FindRawBoneIndex(BoneName);

        if (CurrentBoneIndex == INDEX_NONE)
        {
            continue;
        }

        // --- 여기서부터가 핵심 수정 ---

        // 2. 현재 본(CurrentBoneIndex)의 "애니메이션" 또는 "레퍼런스" 월드 트랜스폼 가져오기
        // 이것은 물리 시뮬레이션 전, 이 본이 *있어야 할* 위치와 회전을 나타냅니다.
        // TempAnimGlobalBoneTransforms는 위에서 계산한, 현재 애니메이션에 따른 글로벌 트랜스폼입니다.
        const FTransform& OriginalBoneWorldTransform_Anim = TempAnimGlobalBoneTransforms[CurrentBoneIndex];

        FTransform NewBoneWorldTransform;
        NewBoneWorldTransform.SetRotation(PhysicsBodyWorldTransform_FromSim.GetRotation()); // 물리 바디의 월드 회전을 가져옴
        NewBoneWorldTransform.NormalizeRotation(); // 회전 정규화


        // 스케일은 항상 1로 설정하여 엿가락 방지
        NewBoneWorldTransform.SetScale3D(FVector::OneVector);

        // 4. 부모 본의 월드 트랜스폼 결정
        FTransform ParentFinalWorldTransform = FTransform::Identity;
        int32 ParentBoneIndex = RefSkeleton.GetParentIndex(CurrentBoneIndex);

        if (ParentBoneIndex != INDEX_NONE)
        {
            // 부모 본이 물리 시뮬레이션 대상인지 확인
            bool bParentIsSimulated = false;
            for (FBodyInstance* TempParentBody : Bodies)
            {
                if (TempParentBody && TempParentBody->GetBodySetup()->BoneName == RefSkeleton.GetBoneName(ParentBoneIndex))
                {
                    ParentFinalWorldTransform = TempAnimGlobalBoneTransforms[ParentBoneIndex]; // 또는 부모의 물리결과가 반영된 값
                    bParentIsSimulated = true; // 이 플래그는 아래 로직에 영향 줄 수 있음
                    break;
                }
            }
            if (!bParentIsSimulated)
            {
                // 부모가 물리 시뮬레이션 대상이 아니면, 애니메이션 포즈의 월드 트랜스폼 사용
                ParentFinalWorldTransform = TempAnimGlobalBoneTransforms[ParentBoneIndex];
            }
        }
        ParentFinalWorldTransform.SetScale3D(FVector::OneVector); // 부모 스케일도 1로.


        // 5. 새로운 로컬 트랜스폼 계산
        //    만약 물리 바디의 위치를 직접 사용하지 않는다면, 현재 본의 애니메이션/레퍼런스 로컬 트랜스폼에서
        //    회전만 물리 결과로 대체하는 방식을 사용합니다.
        FTransform CurrentBoneLocalAnimTransform = RefSkeleton.GetRawRefBonePose()[CurrentBoneIndex]; // 레퍼런스 포즈 또는 애니메이션 로컬 포즈
        if (BonePoseContext.Pose.IsValidIndex(CurrentBoneIndex)) { // 애니메이션이 적용된 로컬 포즈 사용
            CurrentBoneLocalAnimTransform = BonePoseContext.Pose[CurrentBoneIndex];
        }


        // 옵션 1: 물리 바디의 월드 "회전"만 가져와서, 현재 본의 "원래 로컬 위치"와 결합
        FTransform NewFinalBoneLocalTransform;
        // 현재 본의 "애니메이션/레퍼런스" 로컬 위치를 유지
        NewFinalBoneLocalTransform.SetTranslation(CurrentBoneLocalAnimTransform.GetTranslation());
        FQuat PhysicsLocalRotation = ParentFinalWorldTransform.GetRotation().Inverse() * PhysicsBodyWorldTransform_FromSim.GetRotation();
        NewFinalBoneLocalTransform.SetRotation(PhysicsLocalRotation);
        NewFinalBoneLocalTransform.NormalizeRotation();
        NewFinalBoneLocalTransform.SetScale3D(FVector::OneVector); // 로컬 스케일은 항상 1


        // NaN 및 유효성 검사
        if (NewFinalBoneLocalTransform.ContainsNaN() || !NewFinalBoneLocalTransform.IsRotationNormalized())
        {
            // UE_LOG(ELogLevel::Warning, TEXT("UpdatePosePhysics: Invalid local transform for bone %s. Resetting to ref pose local."), *BoneName.ToString());
            BonePoseContext.Pose[CurrentBoneIndex] = RefSkeleton.GetRawRefBonePose()[CurrentBoneIndex];
        }
        else
        {
            BonePoseContext.Pose[CurrentBoneIndex] = NewFinalBoneLocalTransform;
        }
    }
}


void USkeletalMeshComponent::TickPose(float DeltaTime)
{
    if (!ShouldTickAnimation())
    {
        return;
    }

    TickAnimation(DeltaTime);
}

void USkeletalMeshComponent::TickAnimation(float DeltaTime)
{
    if (GetSkeletalMeshAsset())
    {
        TickAnimInstances(DeltaTime);
    }

    CPUSkinning();
}

void USkeletalMeshComponent::TickAnimInstances(float DeltaTime)
{
    if (AnimScriptInstance)
    {
        AnimScriptInstance->UpdateAnimation(DeltaTime, BonePoseContext);
    }
}

bool USkeletalMeshComponent::ShouldTickAnimation() const
{
    if (GEngine->GetWorldContextFromWorld(GetWorld())->WorldType == EWorldType::Editor)
    {
        return false;
    }
    return GetAnimInstance() && SkeletalMeshAsset && SkeletalMeshAsset->GetSkeleton();
}

bool USkeletalMeshComponent::InitializeAnimScriptInstance()
{
    USkeletalMesh* SkelMesh = GetSkeletalMeshAsset();
    
    if (NeedToSpawnAnimScriptInstance())
    {
        AnimScriptInstance = Cast<UAnimInstance>(FObjectFactory::ConstructObject(AnimClass, this));

        if (AnimScriptInstance)
        {
            AnimScriptInstance->InitializeAnimation();
        }
    }
    else
    {
        bool bShouldSpawnSingleNodeInstance = !AnimScriptInstance && SkelMesh && SkelMesh->GetSkeleton();
        if (bShouldSpawnSingleNodeInstance)
        {
            AnimScriptInstance = FObjectFactory::ConstructObject<UAnimSingleNodeInstance>(this);

            if (AnimScriptInstance)
            {
                AnimScriptInstance->InitializeAnimation();
            }
        }
    }

    return true;
}

void USkeletalMeshComponent::ClearAnimScriptInstance()
{
    if (AnimScriptInstance)
    {
        GUObjectArray.MarkRemoveObject(AnimScriptInstance);
    }
    AnimScriptInstance = nullptr;
}

void USkeletalMeshComponent::SetSkeletalMeshAsset(USkeletalMesh* InSkeletalMeshAsset)
{
    if (InSkeletalMeshAsset == GetSkeletalMeshAsset())
    {
        return;
    }
    
    SkeletalMeshAsset = InSkeletalMeshAsset;

    InitAnim();

    BonePoseContext.Pose.Empty();
    RefBonePoseTransforms.Empty();
    AABB = FBoundingBox(InSkeletalMeshAsset->GetRenderData()->BoundingBoxMin, SkeletalMeshAsset->GetRenderData()->BoundingBoxMax);
    
    const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetRefSkeleton();
    BonePoseContext.Pose.InitBones(RefSkeleton.RawRefBoneInfo.Num());
    for (int32 i = 0; i < RefSkeleton.RawRefBoneInfo.Num(); ++i)
    {
        BonePoseContext.Pose[i] = RefSkeleton.RawRefBonePose[i];
        RefBonePoseTransforms.Add(RefSkeleton.RawRefBonePose[i]);
    }
    
    CPURenderData->Vertices = InSkeletalMeshAsset->GetRenderData()->Vertices;
    CPURenderData->Indices = InSkeletalMeshAsset->GetRenderData()->Indices;
    CPURenderData->ObjectName = InSkeletalMeshAsset->GetRenderData()->ObjectName;
    CPURenderData->MaterialSubsets = InSkeletalMeshAsset->GetRenderData()->MaterialSubsets;
    SetSelectedBone(-1);

    /* TODO : 기본적으로 PhysicAsset을 생성하는 대신 bSimulated 옵션이 켜질 때만 PhysicAsset 생성하기 */
    OnCreatePhysicsState();
}

FTransform USkeletalMeshComponent::GetSocketTransform(FName SocketName) const
{
    FTransform Transform = FTransform::Identity;

    if (USkeleton* Skeleton = GetSkeletalMeshAsset()->GetSkeleton())
    {
        int32 BoneIndex = Skeleton->FindBoneIndex(SocketName);

        TArray<FMatrix> GlobalBoneMatrices;
        GetCurrentGlobalBoneMatrices(GlobalBoneMatrices);
        Transform = FTransform(GlobalBoneMatrices[BoneIndex]);
    }
    return Transform;
}

void USkeletalMeshComponent::GetCurrentGlobalBoneMatrices(TArray<FMatrix>& OutBoneMatrices) const
{
    const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetRefSkeleton();
    const int32 BoneNum = RefSkeleton.RawRefBoneInfo.Num();

    OutBoneMatrices.Empty();
    OutBoneMatrices.SetNum(BoneNum);

    for (int32 BoneIndex = 0; BoneIndex < BoneNum; ++BoneIndex)
    {
        // 현재 본의 로컬 변환
        FTransform CurrentLocalTransform = BonePoseContext.Pose[BoneIndex];
        FMatrix LocalMatrix = CurrentLocalTransform.ToMatrixWithScale(); // FTransform -> FMatrix
        
        // 부모 본의 영향을 적용하여 월드 변환 구성
        int32 ParentIndex = RefSkeleton.RawRefBoneInfo[BoneIndex].ParentIndex;
        if (ParentIndex != INDEX_NONE)
        {
            // 로컬 변환에 부모 월드 변환 적용
            LocalMatrix = LocalMatrix * OutBoneMatrices[ParentIndex];
        }
        
        // 결과 행렬 저장
        OutBoneMatrices[BoneIndex] = LocalMatrix;
    }
}

void USkeletalMeshComponent::DEBUG_SetAnimationEnabled(bool bEnable)
{
    bPlayAnimation = bEnable;
    
    if (!bPlayAnimation)
    {
        if (SkeletalMeshAsset && SkeletalMeshAsset->GetSkeleton())
        {
            const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetRefSkeleton();
            BonePoseContext.Pose.InitBones(RefSkeleton.RawRefBonePose.Num());
            for (int32 i = 0; i < RefSkeleton.RawRefBoneInfo.Num(); ++i)
            {
                BonePoseContext.Pose[i] = RefSkeleton.RawRefBonePose[i];
            }
        }
        SetElapsedTime(0.f); 
        CPURenderData->Vertices = SkeletalMeshAsset->GetRenderData()->Vertices;
        CPURenderData->Indices = SkeletalMeshAsset->GetRenderData()->Indices;
        CPURenderData->ObjectName = SkeletalMeshAsset->GetRenderData()->ObjectName;
        CPURenderData->MaterialSubsets = SkeletalMeshAsset->GetRenderData()->MaterialSubsets;
    }
}

void USkeletalMeshComponent::PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping)
{
    SetAnimation(NewAnimToPlay);
    Play(bLooping);
}

int USkeletalMeshComponent::CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const
{
    if (!AABB.Intersect(InRayOrigin, InRayDirection, OutHitDistance))
    {
        return 0;
    }
    if (SkeletalMeshAsset == nullptr)
    {
        return 0;
    }
    
    OutHitDistance = FLT_MAX;
    
    int IntersectionNum = 0;

    const FSkeletalMeshRenderData* RenderData = SkeletalMeshAsset->GetRenderData();

    const TArray<FSkeletalMeshVertex>& Vertices = RenderData->Vertices;
    const int32 VertexNum = Vertices.Num();
    if (VertexNum == 0)
    {
        return 0;
    }
    
    const TArray<UINT>& Indices = RenderData->Indices;
    const int32 IndexNum = Indices.Num();
    const bool bHasIndices = (IndexNum > 0);
    
    int32 TriangleNum = bHasIndices ? (IndexNum / 3) : (VertexNum / 3);
    for (int32 i = 0; i < TriangleNum; i++)
    {
        int32 Idx0 = i * 3;
        int32 Idx1 = i * 3 + 1;
        int32 Idx2 = i * 3 + 2;
        
        if (bHasIndices)
        {
            Idx0 = Indices[Idx0];
            Idx1 = Indices[Idx1];
            Idx2 = Indices[Idx2];
        }

        // 각 삼각형의 버텍스 위치를 FVector로 불러옵니다.
        FVector v0 = FVector(Vertices[Idx0].X, Vertices[Idx0].Y, Vertices[Idx0].Z);
        FVector v1 = FVector(Vertices[Idx1].X, Vertices[Idx1].Y, Vertices[Idx1].Z);
        FVector v2 = FVector(Vertices[Idx2].X, Vertices[Idx2].Y, Vertices[Idx2].Z);

        float HitDistance = FLT_MAX;
        if (IntersectRayTriangle(InRayOrigin, InRayDirection, v0, v1, v2, HitDistance))
        {
            OutHitDistance = FMath::Min(HitDistance, OutHitDistance);
            IntersectionNum++;
        }

    }
    return IntersectionNum;
}

const FSkeletalMeshRenderData* USkeletalMeshComponent::GetCPURenderData() const
{
    return CPURenderData.get();
}

void USkeletalMeshComponent::SetCPUSkinning(bool Flag)
{
    bIsCPUSkinning = Flag;
}

bool USkeletalMeshComponent::GetCPUSkinning()
{
    return bIsCPUSkinning;
}

void USkeletalMeshComponent::SetAnimationMode(EAnimationMode InAnimationMode)
{
    const bool bNeedsChange = AnimationMode != InAnimationMode;
    if (bNeedsChange)
    {
        AnimationMode = InAnimationMode;
        ClearAnimScriptInstance();
    }

    if (GetSkeletalMeshAsset() && (bNeedsChange || AnimationMode == EAnimationMode::AnimationBlueprint))
    {
        InitializeAnimScriptInstance();
    }
}

void USkeletalMeshComponent::InitAnim()
{
    if (GetSkeletalMeshAsset() == nullptr)
    {
        return;
    }

    bool bBlueprintMismatch = AnimClass && AnimScriptInstance && AnimScriptInstance->GetClass() != AnimClass;
    
    const USkeleton* AnimSkeleton = AnimScriptInstance ? AnimScriptInstance->GetCurrentSkeleton() : nullptr;
    
    const bool bClearAnimInstance = AnimScriptInstance && !AnimSkeleton;
    const bool bSkeletonMismatch = AnimSkeleton && (AnimScriptInstance->GetCurrentSkeleton() != GetSkeletalMeshAsset()->GetSkeleton());
    const bool bSkeletonsExist = AnimSkeleton && GetSkeletalMeshAsset()->GetSkeleton() && !bSkeletonMismatch;

    if (bBlueprintMismatch || bSkeletonMismatch || !bSkeletonsExist || bClearAnimInstance)
    {
        ClearAnimScriptInstance();
    }

    const bool bInitializedAnimInstance = InitializeAnimScriptInstance();

    if (bInitializedAnimInstance)
    {
        // TODO: 애니메이션 포즈 바로 반영하려면 여기에서 진행.
    }
}

UPhysicsAsset* USkeletalMeshComponent::GetPhysicsAsset() const
{
    if (SkeletalMeshAsset)
    {
        return SkeletalMeshAsset->GetPhysicsAsset();
    }
    return nullptr;
}

int32 USkeletalMeshComponent::FindRootBodyIndex() const
{
    // Find root physics body
    int32 RootBodyIndex = RootBodyData.BodyIndex;
    if (RootBodyIndex == INDEX_NONE && GetSkeletalMeshAsset())
    {
        if (const UPhysicsAsset* PhysicsAsset = GetPhysicsAsset())
        {
            int32 RawBoneNum = GetSkeletalMeshAsset()->GetSkeleton()->GetRefSkeleton().GetRawBoneNum();

            for (int32 i = 0; i < RawBoneNum; i++)
            {
                int32 BodyInstIndex = PhysicsAsset->FindBodyIndex(GetSkeletalMeshAsset()->GetSkeleton()->GetRefSkeleton().GetBoneName(i));
                if (BodyInstIndex != INDEX_NONE)
                {
                    RootBodyIndex = BodyInstIndex;
                    break;
                }
            }
        }
    }

    return RootBodyIndex;
}


void USkeletalMeshComponent::OnCreatePhysicsState()
{
    if (GetPhysicsAsset() == nullptr)
    {
        return;
    }

    if (GetWorld() == nullptr)
    {
        return;
    }

    InitArticulated(GetOwner()->GetWorld()->GetPhysicsScene());
}

void USkeletalMeshComponent::InitArticulated(FPhysScene* PhysScene)
{
    UPhysicsAsset* const PhysicsAsset = GetPhysicsAsset();

    if (PhysScene == nullptr || PhysicsAsset == nullptr || GetSkeletalMeshAsset() == nullptr)
    {
        return;
    }

    if (Bodies.Num() > 0)
    {
        UE_LOG(ELogLevel::Error, TEXT("USkeletalMeshComponent::InitArticulated : Bodies already created"));
        return;
    }

    /* PhysX에 전달할 Scale 값 */
    FVector Scale3D = GetComponentTransform().GetScale3D();
    const int32 RootBodyIndex = FindRootBodyIndex();

    if (RootBodyIndex == INDEX_NONE)
    {
        UE_LOG(ELogLevel::Error, TEXT("USkeletalMeshComponent::InitArticulated : Could not find root physics body"));
        return;
    }

    // Aggregate 개수 제한 코드 생략
    // TODO : Aggregate 생성 

    

    /* 아래 함수에서 모든 Bodies=BodyInst[] BodySetup으로부터 생성 및 Constraints=ConstraintInst[] 생성 */
    InstantiatePhysicsAsset_Internal(*PhysicsAsset, Scale3D, Bodies, Constraints, PhysScene, this, RootBodyIndex);

    for (int32 BodyIndex = 0; BodyIndex < Bodies.Num(); ++BodyIndex)
    {
        FBodyInstance* Body = Bodies[BodyIndex];
        if (!Body) continue;

        // PhysX에서 쓸 Body(Actor) 이름과 Object, ID 맵핑
    }

    // SetRootBodyIndex(RootBodyIndex);
}

void USkeletalMeshComponent::InstantiatePhysicsAsset_Internal(const UPhysicsAsset& PhysAsset, const FVector& Scale3D, TArray<FBodyInstance*>& OutBodies, TArray<FConstraintInstance*>& OutConstraints, FPhysScene* PhysScene /*= nullptr*/, USkeletalMeshComponent* OwningComponent /*= nullptr*/, int32 UseRootBodyIndex /*= INDEX_NONE*/) const
{
    const float ActualScale = Scale3D.GetAbsMin(); // Scale3D 반영한 BuildScale용 
    const float Scale = ActualScale == 0.f ? KINDA_SMALL_NUMBER : ActualScale;

    TMap<FName, FBodyInstance*> NameToBodyMap;

    InstantiatePhysicsAssetBodies_Internal(PhysAsset, OutBodies, &NameToBodyMap, PhysScene, OwningComponent, UseRootBodyIndex);

    int32 NumOutConstraints = PhysAsset.ConstraintSetup.Num();
    OutConstraints.AddZeroed(NumOutConstraints);

    for (int32 ConstraintIdx = 0; ConstraintIdx < NumOutConstraints; ++ConstraintIdx)
    {
        const UPhysicsConstraintTemplate* ConstraintSetup = PhysAsset.ConstraintSetup[ConstraintIdx]; // 각 Constraint 인스턴스의 원본 : Constraint Template
        FConstraintInstance* ConInst = new FConstraintInstance;
        if (ConstraintSetup == nullptr)
        {
            UE_LOG(ELogLevel::Error, TEXT("USkeletalMeshComponent::InstantiatePhysicsAsset_Internal : ConstraintSetup is NULLPTR"));
            continue;
        }

        OutConstraints[ConstraintIdx] = ConInst;

        ConInst->CopyConstraintParamsFrom(&ConstraintSetup->DefaultInstance);

        ConInst->ConstraintIndex = ConstraintIdx;
        ConInst->PhysScene = PhysScene;


        if (ConstraintSetup == nullptr)
        {
            continue;
        }
        FName Bone1Name = ConstraintSetup->DefaultInstance.ConstraintBone1;
        FName Bone2Name = ConstraintSetup->DefaultInstance.ConstraintBone2;
        FBodyInstance* Body1 = NameToBodyMap.FindRef(Bone1Name);
        FBodyInstance* Body2 = NameToBodyMap.FindRef(Bone2Name);

        /* TODO : 현재는 Scale 적용 코드 생략*/
        // auto ScalePosition = [](const FBodyInstance* InBody, const float InScale, FVector& OutPosition)


        if (Body1 && Body2)
        {
            FConstraintInstance* NewConstraint = new FConstraintInstance();
            ConInst->InitConstraint(Body1, Body2, Scale, OwningComponent);
        }
    }
}

/* 각 BodySetup에 대해 BodyInstance 생성 */
void USkeletalMeshComponent::InstantiatePhysicsAssetBodies_Internal(const UPhysicsAsset& PhysAsset, TArray<FBodyInstance*>& OutBodies, TMap<FName, FBodyInstance*>* OutNameToBodyMap, FPhysScene* PhysScene /*= nullptr*/, USkeletalMeshComponent* OwningComponent /*= nullptr*/, int32 UseRootBodyIndex /*= INDEX_NONE*/)const
{
    const FVector ComponentScale3D = GetComponentTransform().GetScale3D();

    for (int32 i = 0; i < PhysAsset.BodySetup.Num(); ++i)
    {
        UBodySetup* BodySetup = PhysAsset.BodySetup[i];
        if (BodySetup == nullptr || !PhysAsset.GetPreviewMesh() || !PhysAsset.GetPreviewMesh()->GetSkeleton())
        {
            UE_LOG(ELogLevel::Error, TEXT("USkeletalMeshComponent::InstantiatePhysicsAssetBodies_Internal : BodySetup is NULLPTR or other skeleton MISSING"));
            continue;
        }

        const FReferenceSkeleton& RefSkeleton = PhysAsset.GetPreviewMesh()->GetSkeleton()->GetRefSkeleton();
        const int32 BoneIndex = RefSkeleton.FindRawBoneIndex(BodySetup->BoneName);
        if (BoneIndex == INDEX_NONE)
        {
            UE_LOG(ELogLevel::Error, TEXT("USkeletalMeshComponent::InstantiatePhysicsAssetBodies_Internal : Could not find bone index for body '%s'"), *BodySetup->BoneName.ToString());
            continue;
        }

        /* 컴포넌트의 Scale을 적용하여 충돌 형상 정의하기 위함 */
        const FTransform BoneWorldTransform = RefSkeleton.GetRefWorldTransform(BoneIndex);
        BodySetup->ApplyWorldScale(ComponentScale3D);

        FBodyInstance* NewBody = new FBodyInstance();
        NewBody->InitBody(BodySetup, BoneWorldTransform.GetLocation(), PhysScene);

        OutBodies.Add(NewBody);

        if (OutNameToBodyMap)
        {
            OutNameToBodyMap->Add(BodySetup->BoneName, NewBody);
        }
    }
}



bool USkeletalMeshComponent::NeedToSpawnAnimScriptInstance() const
{
    USkeletalMesh* MeshAsset = GetSkeletalMeshAsset();
    USkeleton* AnimSkeleton = MeshAsset ? MeshAsset->GetSkeleton() : nullptr;
    if (AnimationMode == EAnimationMode::AnimationBlueprint && AnimClass && AnimSkeleton)
    {
        if (AnimScriptInstance == nullptr || AnimScriptInstance->GetClass() != AnimClass || AnimScriptInstance->GetOuter() != this)
        {
            return true;
        }
    }
    return false;
}

void USkeletalMeshComponent::CPUSkinning(bool bForceUpdate)
{
    if (bIsCPUSkinning || bForceUpdate)
    {
         QUICK_SCOPE_CYCLE_COUNTER(SkinningPass_CPU)
         const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetSkeleton()->GetRefSkeleton();
         TArray<FMatrix> CurrentGlobalBoneMatrices;
         GetCurrentGlobalBoneMatrices(CurrentGlobalBoneMatrices);
         const int32 BoneNum = RefSkeleton.RawRefBoneInfo.Num();
         
         // 최종 스키닝 행렬 계산
         TArray<FMatrix> FinalBoneMatrices;
         FinalBoneMatrices.SetNum(BoneNum);
    
         for (int32 BoneIndex = 0; BoneIndex < BoneNum; ++BoneIndex)
         {
             FinalBoneMatrices[BoneIndex] = RefSkeleton.InverseBindPoseMatrices[BoneIndex] * CurrentGlobalBoneMatrices[BoneIndex];
         }
         
         const FSkeletalMeshRenderData* RenderData = SkeletalMeshAsset->GetRenderData();
         
         for (int i = 0; i < RenderData->Vertices.Num(); i++)
         {
             FSkeletalMeshVertex Vertex = RenderData->Vertices[i];
             // 가중치 합산
             float TotalWeight = 0.0f;
    
             FVector SkinnedPosition = FVector(0.0f, 0.0f, 0.0f);
             FVector SkinnedNormal = FVector(0.0f, 0.0f, 0.0f);
             
             for (int j = 0; j < 4; ++j)
             {
                 float Weight = Vertex.BoneWeights[j];
                 TotalWeight += Weight;
     
                 if (Weight > 0.0f)
                 {
                     uint32 BoneIdx = Vertex.BoneIndices[j];
                     
                     // 본 행렬 적용 (BoneMatrices는 이미 최종 스키닝 행렬)
                     // FBX SDK에서 가져온 역바인드 포즈 행렬이 이미 포함됨
                     FVector Pos = FinalBoneMatrices[BoneIdx].TransformPosition(FVector(Vertex.X, Vertex.Y, Vertex.Z));
                     FVector4 Norm4 = FinalBoneMatrices[BoneIdx].TransformFVector4(FVector4(Vertex.NormalX, Vertex.NormalY, Vertex.NormalZ, 0.0f));
                     FVector Norm(Norm4.X, Norm4.Y, Norm4.Z);
                     
                     SkinnedPosition += Pos * Weight;
                     SkinnedNormal += Norm * Weight;
                 }
             }
    
             // 가중치 예외 처리
             if (TotalWeight < 0.001f)
             {
                 SkinnedPosition = FVector(Vertex.X, Vertex.Y, Vertex.Z);
                 SkinnedNormal = FVector(Vertex.NormalX, Vertex.NormalY, Vertex.NormalZ);
             }
             else if (FMath::Abs(TotalWeight - 1.0f) > 0.001f && TotalWeight > 0.001f)
             {
                 // 가중치 합이 1이 아닌 경우 정규화
                 SkinnedPosition /= TotalWeight;
                 SkinnedNormal /= TotalWeight;
             }
    
             CPURenderData->Vertices[i].X = SkinnedPosition.X;
             CPURenderData->Vertices[i].Y = SkinnedPosition.Y;
             CPURenderData->Vertices[i].Z = SkinnedPosition.Z;
             CPURenderData->Vertices[i].NormalX = SkinnedNormal.X;
             CPURenderData->Vertices[i].NormalY = SkinnedNormal.Y;
             CPURenderData->Vertices[i].NormalZ = SkinnedNormal.Z;
           }
     }
}

UAnimSingleNodeInstance* USkeletalMeshComponent::GetSingleNodeInstance() const
{
    return Cast<UAnimSingleNodeInstance>(AnimScriptInstance);
}

void USkeletalMeshComponent::SetAnimClass(UClass* NewClass)
{
    SetAnimInstanceClass(NewClass);
}

UClass* USkeletalMeshComponent::GetAnimClass()
{
    return AnimClass;
}

void USkeletalMeshComponent::SetAnimInstanceClass(class UClass* NewClass)
{
    if (NewClass != nullptr)
    {
        // set the animation mode
        const bool bWasUsingBlueprintMode = AnimationMode == EAnimationMode::AnimationBlueprint;
        AnimationMode = EAnimationMode::AnimationBlueprint;

        if (NewClass != AnimClass || !bWasUsingBlueprintMode)
        {
            // Only need to initialize if it hasn't already been set or we weren't previously using a blueprint instance
            AnimClass = NewClass;
            ClearAnimScriptInstance();
            InitAnim();
        }
    }
    else
    {
        // Need to clear the instance as well as the blueprint.
        // @todo is this it?
        AnimClass = nullptr;
        ClearAnimScriptInstance();
    }
}

void USkeletalMeshComponent::SetAnimation(UAnimationAsset* NewAnimToPlay)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetAnimationAsset(NewAnimToPlay, false);
        SingleNodeInstance->SetPlaying(false);

        // TODO: Force Update Pose and CPU Skinning
    }
}

UAnimationAsset* USkeletalMeshComponent::GetAnimation() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetAnimationAsset();
    }
    return nullptr;
}

void USkeletalMeshComponent::Play(bool bLooping)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlaying(true);
        SingleNodeInstance->SetLooping(bLooping);
    }
}

void USkeletalMeshComponent::Stop()
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlaying(false);
    }
}

void USkeletalMeshComponent::SetPlaying(bool bPlaying)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlaying(bPlaying);
    }
}

bool USkeletalMeshComponent::IsPlaying() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->IsPlaying();
    }

    return false;
}

void USkeletalMeshComponent::SetReverse(bool bIsReverse)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetReverse(bIsReverse);
    }
}

bool USkeletalMeshComponent::IsReverse() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->IsReverse();
    }
}

void USkeletalMeshComponent::SetPlayRate(float Rate)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlayRate(Rate);
    }
}

float USkeletalMeshComponent::GetPlayRate() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetPlayRate();
    }

    return 0.f;
}

void USkeletalMeshComponent::SetLooping(bool bIsLooping)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetLooping(bIsLooping);
    }
}

bool USkeletalMeshComponent::IsLooping() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->IsLooping();
    }
    return false;
}

int USkeletalMeshComponent::GetCurrentKey() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetCurrentKey();
    }
    return 0;
}

void USkeletalMeshComponent::SetCurrentKey(int InKey)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetCurrentKey(InKey);
    }
}

void USkeletalMeshComponent::SetElapsedTime(float InElapsedTime)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetElapsedTime(InElapsedTime);
    }
}

float USkeletalMeshComponent::GetElapsedTime() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetElapsedTime();
    }
    return 0.f;
}

int32 USkeletalMeshComponent::GetLoopStartFrame() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetLoopStartFrame();
    }
    return 0;
}

void USkeletalMeshComponent::SetLoopStartFrame(int32 InLoopStartFrame)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetLoopStartFrame(InLoopStartFrame);
    }
}

int32 USkeletalMeshComponent::GetLoopEndFrame() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetLoopEndFrame();
    }
    return 0;
}

void USkeletalMeshComponent::SetLoopEndFrame(int32 InLoopEndFrame)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetLoopEndFrame(InLoopEndFrame);
    }
}

