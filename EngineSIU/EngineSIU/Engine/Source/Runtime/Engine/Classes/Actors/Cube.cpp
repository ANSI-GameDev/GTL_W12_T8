#include "Cube.h"

#include "Components/StaticMeshComponent.h"

#include "Engine/FObjLoader.h"

#include "GameFramework/Actor.h"
#include "PhysicsEngine/BodyInstance.h"
#include "World/World.h"

ACube::ACube()
{
    StaticMeshComponent->SetStaticMesh(FObjManager::CreateStaticMesh(L"Contents/Reference/Reference.obj"));
}

void ACube::InitBodyInstance()
{
    SetActorLocation(FVector(0, 0, 10));

    SetActorTickInEditor(true);
    
    //여기서 값들 넣어보자
    UBodySetup* BodySetup = FObjectFactory::ConstructObject<UBodySetup>(this);
    FKBoxElem Box;
    Box.Center = FVector(0, 0, 0);
    Box.Extent = FVector::OneVector;
    BodySetup->AggGeom.BoxElems.Add(Box);

    
    UWorld* world = GetWorld();
    
    FBodyInstance* CubeBody = new FBodyInstance();
    CubeBody->InitBody(BodySetup, StaticMeshComponent->GetComponentTransform().Translation, world->GetPhysicsScene());
    StaticMeshComponent->SetBodyInstance(CubeBody);
    
}

void ACube::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    //SetActorRotation(GetActorRotation() + FRotator(0, 0, 1));

}
