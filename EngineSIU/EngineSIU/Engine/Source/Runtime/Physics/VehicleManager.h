#pragma once
#include <cooking/PxCooking.h>
#include <vehicle/PxVehicleUpdate.h>
#include <vehicle/PxVehicleUtilControl.h>
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
        SURFACE_TYPE_MAX
    };

    inline float GetTireFrictionMultipliers(uint8 surfaceType, uint8 tireType)
    {
        // Tire model friction for each combination of drivable surface type and tire type.
        static float tireFrictionMultipliers[SURFACE_TYPE_MAX][TIRE_TYPE_MAX]=
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

    struct DigitalInputRawData
    {
        uint8 bAccelKey:1;
        uint8 bBrakeKey:1;
        uint8 bSteerLeftKey:1;
        uint8 bSteerRightKey:1;
        uint8 bHandBrakeKey:1;
        uint8 bGearUpKey:1;
        uint8 bGearDownKey:1;
    };
}

class FVehicleManager
{
public:
    FVehicleManager();
public:
    TArray<physx::PxVehicleWheels*> Vehicles;
    int TargetVehicleIndex;
    VehicleHelper::DigitalInputRawData Inputs;

public:
    void InitPhysXVehicle(physx::PxPhysics* Physics, physx::PxCooking* Cooking);
    void Shutdown();
    void CreateVehicle(physx::PxPhysics* Physics, physx::PxScene* Scene, const physx::PxTransform& StartTransform);
    void Update(float deltaTime, physx::PxScene* Scene);
    void SuspensionRaycasts(physx::PxScene* scene);

private:
    // Cached simulation data of focus vehicle
    physx::PxVehicleWheelsSimData* VehicleSimData;
    physx::PxVehicleDrivableSurfaceToTireFrictionPairs* SurfaceTirePairs;
    
    // Road
    const physx::PxMaterial *RoadMaterial;
    physx::PxVehicleDrivableSurfaceType RoadTypes;

    const physx::PxMaterial *VehicleMaterial;
    
    // chassis
    const float ChassisMass;
    physx::PxConvexMesh* ChassisMesh;

    // wheels
    const float WheelMass;
    physx::PxVec3 WheelCentreOffsets[4];
    physx::PxConvexMesh* WheelMeshes[4];
    uint32 WheelCount;
    uint32 WheelCapacity;

    // query
    TArray<physx::PxVehicleWheelQueryResult> VehicleWheelsQueryResults;
    physx::PxWheelQueryResult* WheelQueryResults;
    
    physx::PxBatchQuery* RaycastBatchQuery;
    // one result for each wheel
    physx::PxRaycastQueryResult* RaycastQueryResults;
    // one hit for each wheel
    physx::PxRaycastHit* RaycastHits;

    
    static VehicleHelper::AABB ComputeMeshAABB(const physx::PxConvexMesh* mesh);

    void ReallocWheelQueryResults();
    void CookPrimitiveMesh(physx::PxPhysics* Physics, physx::PxCooking* Cooking);
    static physx::PxQueryHitType::Enum SampleVehicleWheelRaycastPreFilter(
        physx::PxFilterData filterData0, physx::PxFilterData filterData1, const void* constantBlock,
        physx::PxU32 constantBlockSize, physx::PxHitFlags& queryFlags
    );

    // process inputs
    physx::PxVehicleKeySmoothingData KeySmoothingData = {
        {
            3.0f,	//rise rate eANALOG_INPUT_ACCEL		
            3.0f,	//rise rate eANALOG_INPUT_BRAKE		
            10.0f,	//rise rate eANALOG_INPUT_HANDBRAKE	
            2.5f,	//rise rate eANALOG_INPUT_STEER_LEFT	
            2.5f,	//rise rate eANALOG_INPUT_STEER_RIGHT	
        },
        {
            5.0f,	//fall rate eANALOG_INPUT__ACCEL		
            5.0f,	//fall rate eANALOG_INPUT__BRAKE		
            10.0f,	//fall rate eANALOG_INPUT__HANDBRAKE	
            5.0f,	//fall rate eANALOG_INPUT_STEER_LEFT	
            5.0f	//fall rate eANALOG_INPUT_STEER_RIGHT	
        }
    };
    float SteerVsForwardSpeedData[16] = {
        0.0f,		0.75f,
        5.0f,		0.75f,
        30.0f,		0.125f,
        120.0f,		0.1f,
        PX_MAX_F32, PX_MAX_F32,
        PX_MAX_F32, PX_MAX_F32,
        PX_MAX_F32, PX_MAX_F32,
        PX_MAX_F32, PX_MAX_F32
    };
    physx::PxFixedSizeLookupTable<8> SteerVsForwardSpeedTable;
    void UpdateParameterTargetVehicle(float deltaTime);

    // just hardcoding for debugging
    void UpdateDigitalInput();
};
