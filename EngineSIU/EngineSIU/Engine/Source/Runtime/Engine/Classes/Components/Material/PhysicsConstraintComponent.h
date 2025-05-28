#pragma once
#include "Components/SceneComponent.h"
#include "PhysicsEngine/BodyInstance.h"

struct FConstraintInstance;

class UPhysicsConstraintComponent : public USceneComponent
{
    DECLARE_CLASS(UPhysicsConstraintComponent, USceneComponent)
public:
    UPhysicsConstraintComponent() = default;

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;

    void InitConstraint();
    void SetConstraintInstance(FConstraintInstance* InConstraintInstance) { ConstraintInstance = InConstraintInstance; }
    
    FConstraintInstance* ConstraintInstance = nullptr;

    FBodyInstance* BodyInstance1 = nullptr;
    FBodyInstance* BodyInstance2 = nullptr;
};
