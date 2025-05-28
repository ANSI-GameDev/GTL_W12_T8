#include "SphereActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/FObjLoader.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/BodySetup.h"
#include "World/World.h"

ASphereActor::ASphereActor()
{
    StaticMeshComponent->SetStaticMesh(FObjManager::CreateStaticMesh(L"Contents/Sphere.obj"));
}


void ASphereActor::InitBodyInstance()
{
    SetActorLocation(FVector(0, 0, 10));

    SetActorTickInEditor(true);
    
    //여기서 값들 넣어보자
    UBodySetup* BodySetup = FObjectFactory::ConstructObject<UBodySetup>(this);
    FKSphereElem Sphere(1);
    BodySetup->AggGeom.SphereElems.Add(Sphere);

    
    UWorld* world = GetWorld();
    
    FBodyInstance* SphereBody = new FBodyInstance();
    SphereBody->InitBody(BodySetup, StaticMeshComponent->GetComponentTransform(), world->GetPhysicsScene());
    StaticMeshComponent->SetBodyInstance(SphereBody);
    
}

void ASphereActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
