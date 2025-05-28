#include "CubeActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/FObjLoader.h"
#include "PhysicsEngine/BodyInstance.h"
#include "World/World.h"

ACubeActor::ACubeActor()
{
    StaticMeshComponent->SetStaticMesh(FObjManager::CreateStaticMesh(L"Contents/helloBlender.obj"));
}

void ACubeActor::InitBodyInstance()
{
    SetActorTickInEditor(true);

    FVector Scale = GetActorScale();
    
    //여기서 값들 넣어보자
    UBodySetup* BodySetup = FObjectFactory::ConstructObject<UBodySetup>(this);
    FKBoxElem Box;
    Box.Center = FVector(0);
    Box.Extent = FVector(Scale.X*2, Scale.Y*2, Scale.Z*2);
    BodySetup->AggGeom.BoxElems.Add(Box);
    BodySetup->BoneName = FName(std::to_string(GetUUID()));
    
    UWorld* world = GetWorld();
    
    FBodyInstance* CubeBody = new FBodyInstance();
    CubeBody->InitBody(BodySetup, StaticMeshComponent->GetComponentTransform(), world->GetPhysicsScene());
    StaticMeshComponent->SetBodyInstance(CubeBody);
    
}

void ACubeActor::SetKinematic(bool bIsKinematic)
{
    StaticMeshComponent->GetBodyInstance()->SetRigidbodyKinematic(bIsKinematic);
}

void ACubeActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    //SetActorRotation(GetActorRotation() + FRotator(0, 0, 1));

}
