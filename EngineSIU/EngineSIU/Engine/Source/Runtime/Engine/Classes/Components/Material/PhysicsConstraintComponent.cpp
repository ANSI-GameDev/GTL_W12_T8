#include "PhysicsConstraintComponent.h"

#include "PhysicsEngine/ConstraintInstance.h"
#include "World/World.h"

void UPhysicsConstraintComponent::InitializeComponent()
{
    USceneComponent::InitializeComponent();
}

void UPhysicsConstraintComponent::TickComponent(float DeltaTime)
{
    USceneComponent::TickComponent(DeltaTime);
}

void UPhysicsConstraintComponent::InitConstraint()
{
    if (!BodyInstance1 || !BodyInstance2)
    {
        UE_LOG(ELogLevel::Error, "ConstraintComponent doesn't have BodyInstance");
        return;
    }
    FTransform CompWorld = GetComponentTransform();
    FTransform BodyTransform1 = BodyInstance1->WorldTransform;
    FTransform BodyTransform2 = BodyInstance2->WorldTransform;

    BodyTransform1.Translation = CompWorld.InverseTransformPosition(BodyTransform1.Translation);
    BodyTransform2.Translation = CompWorld.InverseTransformPosition(BodyTransform2.Translation);

    FVector ParentToChildDir = (BodyTransform2.Translation - BodyTransform1.Translation).GetSafeNormal();
    FVector ZAxis = ParentToChildDir;
    FVector Up = FVector::UpVector;
    FVector XAxis = FVector::CrossProduct(Up, ZAxis).GetSafeNormal();
    FVector YAxis = FVector::CrossProduct(ZAxis, XAxis).GetSafeNormal();
    
    //TODO: 축맞추기
    
    ConstraintInstance->InitConstraint(BodyInstance1, BodyInstance2, BodyTransform1, BodyTransform2, GetWorld()->GetPhysicsScene());
}
