
#include "SkeletalMeshActorTest.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/FObjLoader.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Asset/SkeletalMeshAsset.h"

ASkeletalMeshActorTest::ASkeletalMeshActorTest()
{
    SetActorTickInEditor(true);
}

void ASkeletalMeshActorTest::PostSpawnInitialize()
{
    AActor::PostSpawnInitialize();

    USceneComponent* Root = AddComponent<USceneComponent>(FName("RootComponent_0"));
    RootComponent = Root;

    MeshComp = AddComponent<USkeletalMeshComponent>(FName("SkeletalMeshComponent_0"));
    MeshComp->SetSkeletalMeshAsset(UAssetManager::Get().GetSkeletalMesh("Contents/FBX/Sharkry_Unreal"));
    MeshComp->SetupAttachment(RootComponent);

    
    if (MeshComp->GetSkeletalMeshAsset())
    {
        const FReferenceSkeleton& RefSkeleton = MeshComp->GetSkeletalMeshAsset()->GetSkeleton()->GetRefSkeleton();
        
        for (int32 Idx = 0; Idx < RefSkeleton.RawRefBoneInfo.Num(); ++Idx)
        {
            UStaticMeshComponent* Dot = AddComponent<UStaticMeshComponent>();
            Dot->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/SpherePrimitive.obj"));
            DotComponents.Add(Dot);
            
            int32 ParentIndex = RefSkeleton.RawRefBoneInfo[Idx].ParentIndex;
            if (ParentIndex != INDEX_NONE)
            {
                Dot->AttachToComponent(DotComponents[ParentIndex]);
            }
            else
            {
                Dot->AttachToComponent(RootComponent);
            }
            
            Dot->SetRelativeTransform(RefSkeleton.RawRefBonePose[Idx]);
            Dot->SetWorldScale3D(FVector(1.f));
        }
    }
    
}

void ASkeletalMeshActorTest::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);



}
