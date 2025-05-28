#include "CapsuleActor.h"
#include "Components/CapsuleComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/SphylElem.h"
#include "World/World.h"

ACapsuleActor::ACapsuleActor()
{
    CapsuleComponent = AddComponent<UCapsuleComponent>();
    RootComponent = CapsuleComponent;
}

void ACapsuleActor::InitBodyInstance()
{
    SetActorLocation(FVector(0, 0, 10));

    SetActorTickInEditor(true);
    
    //여기서 값들 넣어보자
    UBodySetup* BodySetup = FObjectFactory::ConstructObject<UBodySetup>(this);
    FKSphylElem Capsule = FKSphylElem(1, 1);
    BodySetup->AggGeom.SphylElems.Add(Capsule);

    
    UWorld* world = GetWorld();
    
    FBodyInstance* CapsuleBody = new FBodyInstance();
    CapsuleBody->InitBody(BodySetup, CapsuleComponent->GetComponentTransform(), world->GetPhysicsScene(), GetRootComponent()->GetComponentTransform());
    CapsuleComponent->SetBodyInstance(CapsuleBody);
    
}

UCapsuleComponent* ACapsuleActor::GetShapeComponent() const
{
    return CapsuleComponent;
}
