#pragma once
#include <vehicle/PxVehicleDrive4W.h>

#include "Engine/StaticMeshActor.h"

class AVehicle: public AStaticMeshActor
{
    DECLARE_CLASS(AVehicle, AStaticMeshActor)
public:
    AVehicle();
    void InitVehicle();
    virtual void Tick(float DeltaTime) override;
    virtual void Destroyed() override;
    void RemoveVehicle();

    UPROPERTY_WITH_FLAGS(
        EditAnywhere,
        bool, Targeted
    )

    UPROPERTY_WITH_FLAGS(
        EditAnywhere,
        float, EngineTorqueMax
    )

    UPROPERTY_WITH_FLAGS(
        EditAnywhere,
        float, EngineRotSpeedMax
    )

    UPROPERTY_WITH_FLAGS(
        EditAnywhere,
        float, GearSwitchTime
    )
    
    /** UI */
    int SelectedWheelIndex = 0;
    FVector ModifiedChassisAABBMin;
    FVector ModifiedChassisAABBMax;
    float ChassisMass;
    void ApplyModifiedChassis();
    void ResetPosition();
    float GetWheelRpm();

    // Wheel
    FVector GetWheelPosition(int index);
    float GetWheelRadius(int index);
    float GetWheelWidth(int index);
    void SetWheelPosition(int index, FVector position);
    void SetWheelRadius(int index, float radius);
    void SetWheelWidth(int index, float width);

    // Gear
    uint32 GetCurrentGearNum();
    
private:
    physx::PxVehicleWheelsSimData& GetWheelSimData();
    physx::PxVehicleDrive4W* Vehicle;

    void UpdateProperties();
    void UpdateByModified();
};
