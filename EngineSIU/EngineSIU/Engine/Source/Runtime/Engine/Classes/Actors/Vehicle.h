#pragma once
#include <vehicle/PxVehicleDrive4W.h>

#include "Engine/StaticMeshActor.h"

class AVehicle: public AStaticMeshActor
{
    DECLARE_CLASS(AVehicle, AStaticMeshActor)
public:
    AVehicle();
    void InitVehicle();
    void Tick(float DeltaTime) override;

    UPROPERTY_WITH_FLAGS(
        EditAnywhere,
        bool, Targeted
    );

private:
    physx::PxVehicleDrive4W* Vehicle;
};
