#pragma once
#include <cooking/PxCooking.h>
#include <vehicle/PxVehicleUpdate.h>
#include <vehicle/PxVehicleWheels.h>

#include "Container/Array.h"

namespace VehicleHelper
{
    enum: uint8
    {
        TIRE_TYPE_WETS = 0,
        TIRE_TYPE_MAX
    };

    enum: uint8
    {
        SURFACE_TYPE_ROAD=0,
        SURFACE_TYPES_MAX
    };

    inline float GetTireFrictionMultipliers(uint8 surfaceType, uint8 tireType)
    {
        // Tire model friction for each combination of drivable surface type and tire type.
        static float tireFrictionMultipliers[SURFACE_TYPES_MAX][TIRE_TYPE_MAX]=
        {
            // WETS
            {1.10f},		// ROAD
        };
        return tireFrictionMultipliers[surfaceType][tireType];
    }

    struct AABB
    {
        physx::PxVec3 min;
        physx::PxVec3 max;
    };

    //Collision types and flags describing collision interactions of each collision type.
    enum
    {
        COLLISION_FLAG_GROUND			    =	1 << 0,
        COLLISION_FLAG_WHEEL			    =	1 << 1,
        COLLISION_FLAG_CHASSIS			    =	1 << 2,
        COLLISION_FLAG_OBSTACLE			    =	1 << 3,
        COLLISION_FLAG_DRIVABLE_OBSTACLE    =	1 << 4,

        COLLISION_FLAG_GROUND_AGAINST	            =													COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
        COLLISION_FLAG_WHEEL_AGAINST	            =							COLLISION_FLAG_WHEEL |	COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE,
        COLLISION_FLAG_CHASSIS_AGAINST	            =	COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL |	COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
        COLLISION_FLAG_OBSTACLE_AGAINST	            =	COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL |	COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
        COLLISION_FLAG_DRIVABLE_OBSTACLE_AGAINST    =	COLLISION_FLAG_GROUND 						 |	COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
    };
}

class FVehicleManager
{
public:
    FVehicleManager();
public:
    TArray<physx::PxVehicleWheels*> Vehicles;
    TArray<physx::PxVehicleWheelQueryResult> VehicleWheelsQueryResults;

public:
    void InitPhysXVehicle(physx::PxPhysics* Physics, physx::PxCooking* Cooking);
    void Shutdown();
    void CreateVehicle(physx::PxPhysics* Physics, physx::PxScene* Scene, const physx::PxTransform& StartTransform);

private:
    // Cached simulation data of focus vehicle
    physx::PxVehicleWheelsSimData* VehicleSimData;
    physx::PxVehicleDrivableSurfaceToTireFrictionPairs* SurfaceTirePairs;

    // Road
    const physx::PxMaterial* RoadMaterials;
    physx::PxVehicleDrivableSurfaceType RoadTypes;

    const physx::PxMaterial* VehicleMaterial;
    
    // chassis
    const float ChassisMass;
    physx::PxConvexMesh* ChassisMesh;

    // wheels
    const float WheelMass;
    physx::PxVec3 WheelCentreOffsets[4];
    physx::PxConvexMesh* WheelMeshes[4];
    physx::PxWheelQueryResult* WheelQueryResults;
    uint32 WheelCount;
    uint32 WheelCapacity;
    
    static VehicleHelper::AABB ComputeMeshAABB(const physx::PxConvexMesh* mesh);

    void ReallocWheelQueryResults();
    void CookPrimitiveMesh(physx::PxPhysics* Physics, physx::PxCooking* Cooking);
};
