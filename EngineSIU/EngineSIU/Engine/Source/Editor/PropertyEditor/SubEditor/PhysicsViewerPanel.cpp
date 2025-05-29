#include "PhysicsViewerPanel.h"

#include <PxRigidBody.h>
#include <PxRigidDynamic.h>

#include "FSkeletalMeshDebugger.h"
#include "PhysicsSettingsSerializer.h"
#include "ReferenceSkeleton.h"
#include "UnrealClient.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/EnumClassFlags.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

void PhysicsViewerPanel::Render()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height));

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("PhysicsViewer", nullptr, windowFlags))
    {
        RenderPanelLayout();
        ImGui::End();
    }

    if (!SkeletalMeshComponent || !PrimitiveDrawBatch) return;

    // Skeleton
    if (EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Bone))
    {
        FSkeletalMeshDebugger::DrawSkeleton(SkeletalMeshComponent, PrimitiveDrawBatch);
    }

    // AABBs
    FSkeletalMeshDebugger::DrawSkeletonAABBs(SkeletalMeshComponent, PrimitiveDrawBatch); // 항상 표시됨

    // Constraints (Cone Limits)
    if (EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Constraint) ||
        EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::ConstraintSelectedOnly))
    {
        bool bDrawAllConstraints = EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Constraint);
        FSkeletalMeshDebugger::DrawConeConstraints(SkeletalMeshComponent, PrimitiveDrawBatch, SelectedName, bDrawAllConstraints);
    }

    // Bodies (Capsule OBBs)
    if (EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Body) ||
        EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::BodySelectedOnly))
    {
        bool bDrawAllBodies = EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Body);
        FSkeletalMeshDebugger::DrawBodyShapes(SkeletalMeshComponent, PrimitiveDrawBatch, SelectedName, bDrawAllBodies);
    }
}



void PhysicsViewerPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    if (hWnd && GetClientRect(hWnd, &clientRect))
    {
        Width = static_cast<float>(clientRect.right - clientRect.left);
        Height = static_cast<float>(clientRect.bottom - clientRect.top);
    }
    if (ViewportClient)
    {
        FViewport* Viewport = ViewportClient->GetViewport();
        if (Viewport)
        {
            FRect NewRect(0,0,Width,Height);
            Viewport->ResizeViewport(NewRect);
        }
    }
}
void PhysicsViewerPanel::SetViewportClient(std::shared_ptr<FEditorViewportClient> InViewportClient)
{
    ViewportClient = InViewportClient;
}

void PhysicsViewerPanel::SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
{
    if (SkeletalMeshComponent)
    {
        // 기존 컴포넌트에 대한 정리 작업
        SkeletalMeshComponent->Bodies.Empty();
        // 만약 직접 할당/생성했다면: delete SkeletalMeshComponent;
        // 이 경우 World에서 DetachComponent도 고려 가능
    }

    SkeletalMeshComponent = InSkeletalMeshComponent;

    SkeletalMeshComponent->SetSelectedBone(-1);
    ToggleRagdollSimulation(bSimulateRagdoll);
    if (SkeletalMeshComponent)
    {
        SkeletalMeshComponent->SetSelectedBone(INDEX_NONE);

        // 래그돌 제외 목록 초기화
        SkeletalMeshComponent->ExcludedFromRagdoll.Empty();
    }
}


void PhysicsViewerPanel::SetPrimitiveDrawBatch(UPrimitiveDrawBatch* InPrimitiveDrawBatch)
{
    PrimitiveDrawBatch = InPrimitiveDrawBatch;
}

void PhysicsViewerPanel::RenderViewportPanel()
{
    if (!ViewportClient) return;

    FViewport* Viewport = ViewportClient->GetViewport();
    if (!Viewport) return;

    FViewportResource* Resource = Viewport->GetViewportResource();
    if (!Resource) return;

    FRenderTargetRHI* RenderTarget = Resource->GetRenderTarget(EResourceType::ERT_Compositing);
    if (RenderTarget && RenderTarget->SRV)
    {
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(RenderTarget->SRV), contentSize);
    }
}

void PhysicsViewerPanel::RenderPhysicsSettings()
{
    /*ImGui::Text("Physics Settings:");
    ImGui::Checkbox("Enable Gravity", /* TODO: Hook to simulation setting #1# nullptr);
    ImGui::SliderFloat("Time Step", /* TODO: Hook to sim delta #1# nullptr, 0.001f, 0.033f);*/
}

void PhysicsViewerPanel::RenderInfoPanel()
{
    /*ImGui::Text("Debug Info:");
    ImGui::Text("Body Count: %d", /* TODO: replace with actual data #1# 0);
    ImGui::Text("Broadphase Type: %s", /* TODO: #1# "AABB Grid");*/
}
void PhysicsViewerPanel::RenderBoneRecursive(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, FBaseCompactPose& Pose)
{
    ImGui::PushID(BoneIndex);
    const FName& BoneName = RefSkeleton.GetBoneName(BoneIndex);

    // 현재 Bone의 트리 노드 생성
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    bool bOpen = ImGui::TreeNodeEx(*BoneName.ToString(), flags);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        if (SkeletalMeshComponent)
        {
            SkeletalMeshComponent->SetSelectedBone(BoneIndex);
            SelectedType = EPhysicsSelectionType::Bone;
            SelectedName = BoneName;
        }
    }

    if (bOpen)
    {
        // [1] 자식 Bone 재귀 호출
        for (int32 i = 0; i < RefSkeleton.GetRawBoneNum(); ++i)
        {
            if (RefSkeleton.GetParentIndex(i) == BoneIndex)
            {
                RenderBoneRecursive(RefSkeleton, i, Pose);
            }
        }

        UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetPhysicsAsset();

        if (PhysicsAsset)
        {
            // [2] 현재 Bone에 연결된 Constraint들 출력
            if (EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Constraint))
            {
                for (int32 i = 0; i < PhysicsAsset->ConstraintSetup.Num(); ++i)
                {
                    const UPhysicsConstraintTemplate* Constraint = PhysicsAsset->ConstraintSetup[i];
                    if (!Constraint) continue;

                    const FConstraintInstance& Inst = Constraint->DefaultInstance;

                    if (Inst.ConstraintBone1 == BoneName/* || Inst.ConstraintBone2 == BoneName*/)
                    {
                        FString CName = Inst.JointName.ToString() + TEXT(" [Constraint]");
                        bool bClicked = ImGui::TreeNodeEx(*CName, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                        if (ImGui::IsItemClicked())
                        {
                            SelectedType = EPhysicsSelectionType::Constraint;
                            SelectedName = Inst.JointName;
                            SkeletalMeshComponent->SetSelectedBone(BoneIndex);
                        }
                    }
                }
            }

            // [3] 현재 Bone이 Body를 가지는 경우 출력
            if (EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Body))
            {
                if (PhysicsAsset->FindBodyIndex(BoneName) != INDEX_NONE)
                {
                    FString BodyLabel = BoneName.ToString() + TEXT(" [Body]");
                    ImGui::TreeNodeEx(*BodyLabel, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                    if (ImGui::IsItemClicked())
                    {
                        SelectedType = EPhysicsSelectionType::Body;
                        SelectedName = BoneName;
                        SkeletalMeshComponent->SetSelectedBone(BoneIndex);
                    }
                }
            }
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}



inline void PhysicsViewerPanel::RenderSkeletonUI()
{
    if (!SkeletalMeshComponent || !SkeletalMeshComponent->GetSkeletalMeshAsset())
        return;

    const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetRefSkeleton();
    if (RefSkeleton.GetRawBoneNum() == 0)
        return;

    float rightW = Width * 0.25f;
    float panelX = Width - rightW;
    float panelY = 0.0f;

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightW, Height), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Skeleton Hierarchy", nullptr, flags))
    {
        FBaseCompactPose& Pose = SkeletalMeshComponent->BonePoseContext.Pose;

        const auto& SkeletalMeshes = UAssetManager::Get().GetSkeletalMeshMap();
        static int CurrentIndex = -1;
        static TArray<FName> MeshNames;
        MeshNames.Empty();
        for (auto& Pair : SkeletalMeshes) MeshNames.Add(Pair.Key);

        if (ImGui::BeginCombo("SkeletalMesh", (CurrentIndex >= 0 && MeshNames.IsValidIndex(CurrentIndex)) ? *MeshNames[CurrentIndex].ToString() : "None"))
        {
            for (int i = 0; i < MeshNames.Num(); ++i)
            {
                bool bSelected = (i == CurrentIndex);
                if (ImGui::Selectable(*MeshNames[i].ToString(), bSelected))
                {
                    CurrentIndex = i;
                    SkeletalMeshComponent->SetSkeletalMeshAsset(UAssetManager::Get().GetSkeletalMesh(MeshNames[i]));
                    ToggleRagdollSimulation(bSimulateRagdoll);
                    // 초기화 등 필요한 작업
                }
                if (bSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetRawBoneNum(); ++BoneIndex)
        {
            if (RefSkeleton.GetParentIndex(BoneIndex) == INDEX_NONE)
            {
                RenderBoneRecursive(RefSkeleton, BoneIndex, Pose);
            }
        }

        RenderSelectedProperty(Pose);
        /*
        int SelectedBoneIndex = SkeletalMeshComponent->GetSelectedBone();
        if (SelectedBoneIndex != INDEX_NONE && Pose.IsValidIndex(SelectedBoneIndex))
        {
            ImGui::SeparatorText("Bone Transform");

            // 편집할 Transform 복사
            FTransform BoneTransform = Pose.GetBoneTransform(SelectedBoneIndex);
            FVector Translation = BoneTransform.GetTranslation();
            FQuat RotationQuat = BoneTransform.GetRotation();
            FRotator Rotator = RotationQuat.Rotator();

            static int32 LastBoneIndex = -1;
            static float EulerAngles[3] = { 0.f, 0.f, 0.f };

            // 본이 바뀌면 회전값 초기화
            if (SelectedBoneIndex != LastBoneIndex)
            {
                EulerAngles[0] = Rotator.Roll;
                EulerAngles[1] = Rotator.Yaw;
                EulerAngles[2] = Rotator.Pitch;
                LastBoneIndex = SelectedBoneIndex;
            }

            float pos[3] = { Translation.X, Translation.Y, Translation.Z };
            bool bChanged = false;

            if (ImGui::DragFloat3("Position", pos, 0.1f))
            {
                BoneTransform.SetTranslation(FVector(pos[0], pos[1], pos[2]));
                bChanged = true;
            }

            if (ImGui::DragFloat3("Rotation", EulerAngles, 0.5f))
            {
                FRotator NewRotator(EulerAngles[0], EulerAngles[1], EulerAngles[2]); // Roll, Yaw,Pitch  순서

                BoneTransform.SetRotation(FQuat(NewRotator));
                bChanged = true;
            }

            if (bChanged)
            {
                Pose.SetBoneTransform(SelectedBoneIndex, BoneTransform);
            }
        }
        */

    }

    ImGui::End();
}
void ApplyConstraintLimit(FTransform& BoneTransform, const FReferenceSkeleton& RefSkeleton, UPhysicsAsset* PhysicsAsset, const FName& BoneName, TArray<FTransform> BonePoseLocal)
{
    if (!PhysicsAsset) return;

    int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
    if (BoneIndex == INDEX_NONE) return;

    // 🛠️ 1. BoneTransform을 Pose에 반영해줌
    BonePoseLocal[BoneIndex] = BoneTransform;

    // [1] 로컬 -> 월드 회전 변환
    FTransform WorldTransform = BonePoseLocal[BoneIndex];
    int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
    while (ParentIndex != INDEX_NONE)
    {
        WorldTransform = BonePoseLocal[ParentIndex] * WorldTransform;
        ParentIndex = RefSkeleton.GetParentIndex(ParentIndex);
    }

    FQuat TargetWorldQuat = WorldTransform.GetRotation();
    FQuat RefQuat = RefSkeleton.GetRefWorldTransform(BoneIndex).GetRotation();
    FQuat DeltaQuat = RefQuat.Inverse() * TargetWorldQuat;

    for (UPhysicsConstraintTemplate* Constraint : PhysicsAsset->ConstraintSetup)
    {
        const FConstraintInstance& Inst = Constraint->DefaultInstance;
        if (Inst.ConstraintBone1 != BoneName)
            continue;

        const FVector TwistAxis = RefQuat.GetUnitAxis(EAxis::X); // 언리얼 기준 Twist = X
        FVector RotationAxis;
        float AngleRad;
        DeltaQuat.ToAxisAndAngle(RotationAxis, AngleRad);
        RotationAxis.Normalize();

        float SwingDot = FVector::DotProduct(RotationAxis, TwistAxis);
        FVector TwistComponent = TwistAxis * SwingDot;
        FVector SwingComponent = RotationAxis - TwistComponent;

        FQuat ClampedDeltaQuat = DeltaQuat;

        // Twist 제한
        if (Inst.ProfileInstance.TwistLimit.TwistMotion == EAngularConstraintMotion::ACM_Limited)
        {
            float TwistLimitRad = FMath::DegreesToRadians(Inst.ProfileInstance.TwistLimit.TwistLimitDegrees);
            float TwistAngle = SwingDot * AngleRad;
            TwistAngle = FMath::Clamp(TwistAngle, -TwistLimitRad, TwistLimitRad);
            FQuat TwistQuat = FQuat(TwistAxis, TwistAngle);

            // Swing 제한
            float SwingAngle = (SwingComponent.Size() > KINDA_SMALL_NUMBER) ? (AngleRad * (1 - FMath::Abs(SwingDot))) : 0.0f;
            float SwingLimit1 = FMath::DegreesToRadians(Inst.ProfileInstance.ConeLimit.Swing1LimitDegrees);
            float SwingLimit2 = FMath::DegreesToRadians(Inst.ProfileInstance.ConeLimit.Swing2LimitDegrees);
            float SwingLimit = FMath::Min(SwingLimit1, SwingLimit2);
            SwingAngle = FMath::Clamp(SwingAngle, 0.0f, SwingLimit);
            FQuat SwingQuat = FQuat(SwingComponent.GetSafeNormal(), SwingAngle);

            ClampedDeltaQuat = SwingQuat * TwistQuat;
        }

        // [2] 최종 월드 회전
        FQuat NewWorldQuat = (RefQuat * ClampedDeltaQuat).GetNormalized();

        // [3] 부모 기준 로컬 회전으로 되돌림
        FQuat ParentWorldQuat = FQuat::Identity;
        int32 Parent = RefSkeleton.GetParentIndex(BoneIndex);
        if (Parent != INDEX_NONE)
        {
            FTransform ParentWorld = BonePoseLocal[Parent];
            int32 Ancestor = RefSkeleton.GetParentIndex(Parent);
            while (Ancestor != INDEX_NONE)
            {
                ParentWorld = BonePoseLocal[Ancestor] * ParentWorld;
                Ancestor = RefSkeleton.GetParentIndex(Ancestor);
            }
            ParentWorldQuat = ParentWorld.GetRotation();
        }

        FQuat LocalQuat = ParentWorldQuat.Inverse() * NewWorldQuat;
        BoneTransform.SetRotation(LocalQuat.GetNormalized());
        break;
    }
}
template <typename ElemType>
void RenderCollisionShapeArray(const char* LabelPrefix, TArray<ElemType>& ElemArray)
{
    static int32 LastDeletedIndex = -1;

    for (int32 i = 0; i < ElemArray.Num(); )
    {
        bool bDeleted = false;

        FString ShapeLabel = FString::Printf(TEXT("%s [%d]"), LabelPrefix, i);

        // 🟡 삭제 직후인 항목은 자동으로 접히도록
        if (i == LastDeletedIndex)
        {
            ImGui::SetNextItemOpen(false, ImGuiCond_Always);
            LastDeletedIndex = -1; // 한 번만 적용되도록 리셋
        }

        if (ImGui::TreeNode(*ShapeLabel))
        {
            ElemType& Elem = ElemArray[i];

            FVector Center = Elem.Center;
            float CenterArr[3] = { Center.X, Center.Y, Center.Z };
            if (ImGui::DragFloat3("Center", CenterArr, 0.1f))
                Elem.Center = FVector(CenterArr[0], CenterArr[1], CenterArr[2]);

            if constexpr (std::is_same_v<std::decay_t<ElemType>, FKSphylElem>)
            {
                FVector Euler = Elem.RQuat.Rotator().Euler();
                static float LastEuler[3] = { 0.f, 0.f, 0.f };
                LastEuler[0] = Euler.X; LastEuler[1] = Euler.Y; LastEuler[2] = Euler.Z;

                if (ImGui::DragFloat3("Rotation", LastEuler, 0.5f))
                    Elem.RQuat = FQuat::MakeFromEuler(FVector(LastEuler[0], LastEuler[1], LastEuler[2]));

                ImGui::DragFloat("Radius", &Elem.Radius, 0.1f, 0.01f, 1000.f);
                ImGui::DragFloat("Length", &Elem.Length, 0.1f, 0.01f, 1000.f);
            }
            else if constexpr (std::is_same_v<std::decay_t<ElemType>, FKSphereElem>)
            {
                ImGui::DragFloat("Radius", &Elem.Radius, 0.1f, 0.01f, 1000.f);
            }
            else if constexpr (std::is_same_v<std::decay_t<ElemType>, FKBoxElem>)
            {
                FVector Ext = Elem.Extent;
                float ExtArr[3] = { Ext.X, Ext.Y, Ext.Z };
                if (ImGui::DragFloat3("Extent", ExtArr, 0.1f))
                    Elem.Extent = FVector(ExtArr[0], ExtArr[1], ExtArr[2]);

                FVector RotEuler = Elem.Rotation.Euler();
                float RotArr[3] = { RotEuler.X, RotEuler.Y, RotEuler.Z };
                if (ImGui::DragFloat3("Rotation", RotArr, 0.5f))
                    Elem.Rotation = FRotator(FVector(RotArr[0], RotArr[1], RotArr[2]));
            }

            if (ImGui::Button("Delete"))
            {
                bDeleted = true;
            }

            ImGui::TreePop();
        }

        if (bDeleted)
        {
            ElemArray.RemoveAt(i);
            LastDeletedIndex = i; // 다음 인덱스를 강제로 닫게 설정
        }
        else
        {
            ++i;
        }
    }
}


void PhysicsViewerPanel::RenderSelectedProperty(FBaseCompactPose& Pose)
{
    if (!SkeletalMeshComponent) return;

    UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetPhysicsAsset();
    const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetRefSkeleton();
    int32 BoneIndex = RefSkeleton.FindBoneIndex(SelectedName);
    FString SelectedLabel = SelectedName.ToString();
    // Bone 관련 정보는 항상 계산

    // 공통: 선택한 이름 표시
    ImGui::SeparatorText("Selection Info");
    if (ImGui::Button("Save Physics Settings"))
    {
        PhysicsSettingsSerializer::SavePhysicsSettings(SkeletalMeshComponent);
    }
    ImGui::Text("Selected Name: %s", *SelectedLabel);
    ImGui::Text("Type: %s",
        SelectedType == EPhysicsSelectionType::Bone ? "Bone" :
        SelectedType == EPhysicsSelectionType::Body ? "Body" :
        SelectedType == EPhysicsSelectionType::Constraint ? "Constraint" : "None"
    );
    if (SelectedType == EPhysicsSelectionType::Body)
    {
        int32 BodyIndex = PhysicsAsset ? PhysicsAsset->FindBodyIndex(SelectedName) : INDEX_NONE;
        if (PhysicsAsset && PhysicsAsset->BodySetup.IsValidIndex(BodyIndex))
        {
            BoneIndex = RefSkeleton.FindBoneIndex(SelectedName); // Body 이름이 Bone 이름과 동일하다고 가정
        }
    }
    else if (SelectedType == EPhysicsSelectionType::Constraint)
    {
        for (UPhysicsConstraintTemplate* Constraint : PhysicsAsset->ConstraintSetup)
        {
            if (Constraint && Constraint->DefaultInstance.JointName == SelectedName)
            {
                BoneIndex = RefSkeleton.FindBoneIndex(Constraint->DefaultInstance.ConstraintBone1);
                break;
            }
        }
    }
    //Bone 관련 정보 표시
    if (BoneIndex != INDEX_NONE && Pose.IsValidIndex(BoneIndex))
    {
        ImGui::SeparatorText("Bone Transform (Editable)");

        // Editable CompactPose Transform (Current Pose)
        FTransform BoneTransform = Pose.GetBoneTransform(BoneIndex);
        FVector Translation = BoneTransform.GetTranslation();
        FRotator Rotator = BoneTransform.GetRotation().Rotator();

        static int32 LastBoneIndex = -1;
        static float EulerAngles[3] = { 0.f, 0.f, 0.f };

        if (LastBoneIndex != BoneIndex)
        {
            EulerAngles[0] = Rotator.Roll;
            EulerAngles[1] = Rotator.Yaw;
            EulerAngles[2] = Rotator.Pitch;
            LastBoneIndex = BoneIndex;
        }

        float pos[3] = { Translation.X, Translation.Y, Translation.Z };
        bool bChanged = false;

        if (ImGui::DragFloat3("Local Position", pos, 0.1f))
        {
            BoneTransform.SetTranslation(FVector(pos[0], pos[1], pos[2]));
            bChanged = true;
        }

        if (ImGui::DragFloat3("Local Rotation", EulerAngles, 0.5f))
        {
            FQuat NewQuat = FQuat(FRotator(EulerAngles[0], EulerAngles[1], EulerAngles[2]));
            BoneTransform.SetRotation(NewQuat);

            // ✅ Constraint 적용
            ApplyConstraintLimit(BoneTransform, RefSkeleton, PhysicsAsset, SelectedName, Pose.GetBones());

            bChanged = true;
        }


        if (bChanged)
        {
            Pose.SetBoneTransform(BoneIndex, BoneTransform);
            // Shape 위치도 함께 반영
            if (SkeletalMeshComponent && SkeletalMeshComponent->Bodies.Contains(SelectedName))
            {
                FBodyInstance* BodyInstance = SkeletalMeshComponent->Bodies[SelectedName];
                if (BodyInstance && BodyInstance->RigidBody && BodyInstance->bSimulatePhysics == false)
                {
                    const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetRefSkeleton();

                    // [1] 부모 트랜스폼 누적
                    FTransform WorldTransform = BoneTransform;
                    int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
                    while (ParentIndex != INDEX_NONE && Pose.IsValidIndex(ParentIndex))
                    {
                        WorldTransform = Pose.GetBoneTransform(ParentIndex) * WorldTransform;
                        ParentIndex = RefSkeleton.GetParentIndex(ParentIndex);
                    }

                    // [2] 적용
                    BodyInstance->SetTransformRigidBody(WorldTransform);
                }
            }
        }

        // World Position (Current Pose)
        TArray<FMatrix> GlobalMatrices;
        SkeletalMeshComponent->GetCurrentGlobalBoneMatrices(GlobalMatrices);

        if (GlobalMatrices.IsValidIndex(BoneIndex))
        {
            if (ImGui::TreeNode("World Transform (Current)")) {
                const FMatrix BoneMatrix = GlobalMatrices[BoneIndex];
                const FVector WorldPos = BoneMatrix.GetOrigin();
                const FQuat WorldQuat = BoneMatrix.ToQuat();
                const FRotator WorldRot = WorldQuat.Rotator();

                ImGui::Text("World Position: (%.2f, %.2f, %.2f)", WorldPos.X, WorldPos.Y, WorldPos.Z);
                ImGui::Text("World Rotation (Euler): (Roll=%.1f, Pitch=%.1f, Yaw=%.1f)", WorldRot.Roll, WorldRot.Pitch, WorldRot.Yaw);
                ImGui::Text("World Rotation (Quat): (X=%.3f, Y=%.3f, Z=%.3f, W=%.3f)", WorldQuat.X, WorldQuat.Y, WorldQuat.Z, WorldQuat.W);
                ImGui::TreePop();
            }
        }

        const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetRefSkeleton();
        if (RefSkeleton.IsValidRawIndex(BoneIndex))
        {
            if (ImGui::TreeNode("RefSkeleton Transform")) {

                const FTransform RefWorld = RefSkeleton.GetRefWorldTransform(BoneIndex);
                const FVector RefWorldPos = RefWorld.GetTranslation();
                const FQuat RefWorldQuat = RefWorld.GetRotation();
                const FRotator RefWorldRot = RefWorldQuat.Rotator();

                ImGui::Text("Ref World Pos: (%.2f, %.2f, %.2f)", RefWorldPos.X, RefWorldPos.Y, RefWorldPos.Z);
                ImGui::Text("Ref World Rot (Euler): (Roll=%.1f, Pitch=%.1f, Yaw=%.1f)", RefWorldRot.Roll, RefWorldRot.Pitch, RefWorldRot.Yaw);
                ImGui::Text("Ref World Rot (Quat): (X=%.3f, Y=%.3f, Z=%.3f, W=%.3f)", RefWorldQuat.X, RefWorldQuat.Y, RefWorldQuat.Z, RefWorldQuat.W);
                ImGui::TreePop();
            }
        }

    }

    // === 기존 Body 처리 영역 ===
    if (SelectedType == EPhysicsSelectionType::Body)
    {
        if (!PhysicsAsset) return;
        int32 BodyIndex = PhysicsAsset->FindBodyIndex(SelectedName);
        if (BodyIndex == INDEX_NONE) return;

        UBodySetup* BodySetup = PhysicsAsset->BodySetup[BodyIndex];
        if (!BodySetup) return;
        FBodyInstance* BodyInstance = SkeletalMeshComponent->Bodies[BodySetup->BoneName];
        bool bAffectByRagdoll = BodyInstance->bSimulatePhysics;
        if (ImGui::Checkbox("Affect by Ragdoll", &bAffectByRagdoll))
        {
            BodyInstance->SetbSimulatePhysics(bAffectByRagdoll);

            if (bAffectByRagdoll)
            {
                SkeletalMeshComponent->ExcludedFromRagdoll.Remove(BodySetup->BoneName);
                BodyInstance->RigidBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
            }
            else
            {
                SkeletalMeshComponent->ExcludedFromRagdoll.Add(BodySetup->BoneName);
                BodyInstance->RigidBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
                /*FTransform PoseT = SkeletalMeshComponent->BonePoseContext.Pose[BoneIndex];
                FTransform ParentWorld = SkeletalMeshComponent->GetParentWorldTransform(BoneIndex);
                BodyInstance->SetTransformRigidBody(PoseT * ParentWorld);*/
            }
        }

        ImGui::SeparatorText("Body Collision Shapes");

        if (ImGui::Button("Add Sphere"))
        {
            FKSphereElem NewElem(10.0f); // default radius
            BodySetup->AggGeom.SphereElems.Add(NewElem);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Capsule"))
        {
            FKSphylElem NewElem(10.0f, 20.0f); // radius, length
            BodySetup->AggGeom.SphylElems.Add(NewElem);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Box"))
        {
            FKBoxElem NewElem(FVector(10.0f, 10.0f, 10.0f));
            BodySetup->AggGeom.BoxElems.Add(NewElem);
        }
        /*ImGui::SameLine();
        if (ImGui::Button("Add Convex"))
        {
            FKConvexElem NewElem;
            // 최소 버텍스 세트는 외부 입력 필요
            BodySetup->AggGeom.ConvexElems.Add(NewElem);
        }*/

        RenderCollisionShapeArray("Sphere", BodySetup->AggGeom.SphereElems);
        RenderCollisionShapeArray("Capsule", BodySetup->AggGeom.SphylElems);
        RenderCollisionShapeArray("Box", BodySetup->AggGeom.BoxElems);
        //RenderCollisionShapeArray("Convex", BodySetup->AggGeom.ConvexElems);
    }


    // === 기존 Constraint 처리 영역 ===
    else if (SelectedType == EPhysicsSelectionType::Constraint)
    {
        if (!PhysicsAsset) return;

        for (UPhysicsConstraintTemplate* Constraint : PhysicsAsset->ConstraintSetup)
        {
            if (Constraint && Constraint->DefaultInstance.JointName == SelectedName)
            {
                ImGui::SeparatorText("Constraint Settings");

                FConstraintInstance& Inst = Constraint->DefaultInstance;

                float LinearLimit = Inst.ProfileInstance.LinearLimit.Limit;
                if (ImGui::DragFloat("Limit", &LinearLimit, 0.1f, 0.0f, 1000.0f))
                    Inst.ProfileInstance.LinearLimit.Limit = LinearLimit;

                const char* LinearMotionTypes[] = { "Free", "Limited", "Locked" };
                int xMotion = Inst.ProfileInstance.LinearLimit.XMotion;
                int yMotion = Inst.ProfileInstance.LinearLimit.YMotion;
                int zMotion = Inst.ProfileInstance.LinearLimit.ZMotion;

                ImGui::Combo("X Motion", &xMotion, LinearMotionTypes, 3);
                ImGui::Combo("Y Motion", &yMotion, LinearMotionTypes, 3);
                ImGui::Combo("Z Motion", &zMotion, LinearMotionTypes, 3);

                Inst.ProfileInstance.LinearLimit.XMotion = (ELinearConstraintMotion)xMotion;
                Inst.ProfileInstance.LinearLimit.YMotion = (ELinearConstraintMotion)yMotion;
                Inst.ProfileInstance.LinearLimit.ZMotion = (ELinearConstraintMotion)zMotion;

                ImGui::Separator();
                ImGui::Text("Swing (Cone) Constraint");

                float Swing1 = Inst.ProfileInstance.ConeLimit.Swing1LimitDegrees;
                float Swing2 = Inst.ProfileInstance.ConeLimit.Swing2LimitDegrees;

                if (ImGui::DragFloat("Swing1 (Y)", &Swing1, 0.1f, 0.0f, 180.0f))
                    Inst.ProfileInstance.ConeLimit.Swing1LimitDegrees = Swing1;
                if (ImGui::DragFloat("Swing2 (X)", &Swing2, 0.1f, 0.0f, 180.0f))
                    Inst.ProfileInstance.ConeLimit.Swing2LimitDegrees = Swing2;

                int swing1Motion = Inst.ProfileInstance.ConeLimit.Swing1Motion;
                int swing2Motion = Inst.ProfileInstance.ConeLimit.Swing2Motion;
                const char* AngularMotionTypes[] = { "Free", "Limited", "Locked" };

                ImGui::Combo("Swing1 Motion", &swing1Motion, AngularMotionTypes, 3);
                ImGui::Combo("Swing2 Motion", &swing2Motion, AngularMotionTypes, 3);

                Inst.ProfileInstance.ConeLimit.Swing1Motion = (EAngularConstraintMotion)swing1Motion;
                Inst.ProfileInstance.ConeLimit.Swing2Motion = (EAngularConstraintMotion)swing2Motion;

                ImGui::Separator();
                ImGui::Text("Twist Constraint");

                float Twist = Inst.ProfileInstance.TwistLimit.TwistLimitDegrees;
                if (ImGui::DragFloat("Twist Limit (Z)", &Twist, 0.1f, 0.0f, 180.0f))
                    Inst.ProfileInstance.TwistLimit.TwistLimitDegrees = Twist;

                int twistMotion = Inst.ProfileInstance.TwistLimit.TwistMotion;
                ImGui::Combo("Twist Motion", &twistMotion, AngularMotionTypes, 3);
                Inst.ProfileInstance.TwistLimit.TwistMotion = (EAngularConstraintMotion)twistMotion;

                ImGui::Separator();
                ImGui::Text("Connected Bones:");
                ImGui::BulletText("Bone1: %s", *Inst.ConstraintBone1.ToString());
                ImGui::BulletText("Bone2: %s", *Inst.ConstraintBone2.ToString());

                break;
            }
        }
    }
}

inline void PhysicsViewerPanel::RenderPanelLayout()
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = Width * 0.75f;
    float topBarHeight = 40.0f; // 상단 디버그 버튼 영역 높이

    // 1. 상단 바 UI 렌더링 (Physics Debug 버튼)
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftW, topBarHeight), ImGuiCond_Always);
    if (ImGui::Begin("TopBar", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse))
    {
        DrawShowFlags();
    }
    ImGui::End();

    // 2. 실제 PhysicsCanvas 뷰포트 영역
    ImGui::SetNextWindowPos(ImVec2(0, topBarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftW, Height - topBarHeight), ImGuiCond_Always);

    ImGuiWindowFlags canvasFlags =
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    if (ImGui::Begin("PhysicsCanvas", nullptr, canvasFlags))
    {
        RenderViewportPanel(); // SRV 그리기
        ImGui::Separator();
        RenderPhysicsSettings();
        ImGui::Separator();
        RenderInfoPanel();
    }
    ImGui::End();

    // 3. 우측 Bone 트리
    RenderSkeletonUI();
}

void PhysicsViewerPanel::ToggleFlag(EPhysicsDebugDisplay Flag, bool bEnable)
{
    if (bEnable)
    {
        DebugDisplayFlags |= Flag;
    }
    else
    {
        DebugDisplayFlags &= ~Flag;
    }
}

void PhysicsViewerPanel::DrawShowFlags()
{
    if (ImGui::Button("Physics Debug", ImVec2(120, 32)))
    {
        ImGui::OpenPopup("PhysicsDebugFlags");
    }

    if (ImGui::BeginPopup("PhysicsDebugFlags"))
    {
        bool bBone = EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Bone);
        bool bBody = EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Body);
        bool bBodySelected = EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::BodySelectedOnly);
        bool bConstraint = EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Constraint);
        bool bConstraintSelected = EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::ConstraintSelectedOnly);

        if (ImGui::Checkbox("Show Bones", &bBone))
        {
            ToggleFlag(EPhysicsDebugDisplay::Bone, bBone);
        }
        if (ImGui::Checkbox("Show All Bodies", &bBody))
        {
            ToggleFlag(EPhysicsDebugDisplay::Body, bBody);
        }
        if (ImGui::Checkbox("Show Selected Body Only", &bBodySelected))
        {
            ToggleFlag(EPhysicsDebugDisplay::BodySelectedOnly, bBodySelected);
        }
        if (ImGui::Checkbox("Show All Constraints", &bConstraint))
        {
            ToggleFlag(EPhysicsDebugDisplay::Constraint, bConstraint);
        }
        if (ImGui::Checkbox("Show Selected Constraint Only", &bConstraintSelected))
        {
            ToggleFlag(EPhysicsDebugDisplay::ConstraintSelectedOnly, bConstraintSelected);
        }

        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Start Ragdoll", ImVec2(120, 32)))
    {
        ToggleRagdollSimulation(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Ragdoll", ImVec2(120, 32)))
    {
        ToggleRagdollSimulation(false);
    }

}
void PhysicsViewerPanel::ToggleRagdollSimulation(bool bEnable)
{
    if (!SkeletalMeshComponent || !SkeletalMeshComponent->GetPhysicsAsset())
        return;

    USkeleton* Skeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton();
    const FReferenceSkeleton& RefSkeleton = Skeleton->GetRefSkeleton();
    const TArray<FTransform>& RefPose = Skeleton->GetReferencePose();

    for (auto& Pair : SkeletalMeshComponent->Bodies)
    {
        const FName& BoneName = Pair.Key;
        FBodyInstance* Instance = Pair.Value;
        if (!Instance) continue;

        // ❗ 제외된 본은 건너뛴다
        if (SkeletalMeshComponent->ExcludedFromRagdoll.Contains(BoneName))
            continue;

        int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
        if (!RefPose.IsValidIndex(BoneIndex)) continue;

        if (bEnable)
        {
            Instance->RigidBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);

            Instance->RigidBody->setLinearVelocity({ 0, 0, 0 });
            Instance->RigidBody->setAngularVelocity({ 0, 0, 0 });
        }
        else
        {
            Instance->RigidBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
            FTransform RefWorld = RefSkeleton.GetRefWorldTransform(BoneIndex);
            Instance->SetTransformRigidBody(RefWorld);
        }
    }


    bSimulateRagdoll = bEnable;
}
