#include "VehicleManager.h"

#include <PxPhysics.h>
#include <PxSceneLock.h>
#include <cooking/PxCooking.h>
#include <extensions/PxDefaultStreams.h>
#include <extensions/PxRigidActorExt.h>
#include <vehicle/PxVehicleDrive4W.h>
#include <vehicle/PxVehicleDriveTank.h>
#include <vehicle/PxVehicleUtil.h>
#include <vehicle/PxVehicleUtilControl.h>
#include <vehicle/PxVehicleUtilSetup.h>
using namespace physx;

FVehicleManager::FVehicleManager()
    : VehicleSimData(nullptr)
    , TargetVehicleIndex(-1)
    , SurfaceTirePairs(nullptr)
    , RoadMaterials(nullptr)
    , RoadTypes()
    , VehicleMaterial(nullptr)
    , WheelQueryResults(nullptr)
    , RaycastBatchQuery(nullptr)
    , RaycastQueryResults(nullptr)
    , RaycastHits(nullptr)
    , ChassisMass(1500.f)
    , ChassisMesh(nullptr)
    , WheelMass(20.f)
    , WheelMeshes{}
    , WheelCount(0)
    , WheelCapacity(0)
{
    SteerVsForwardSpeedTable = PxFixedSizeLookupTable<8>(SteerVsForwardSpeedData, 4);
    
    // TODO: 메쉬 받아서 계산하기
    WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT] = PxVec3(15, -10, -5);
    WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT] = PxVec3(15, 10, -5);
    WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT] = PxVec3(-15, -10, -5);
    WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_RIGHT] = PxVec3(-15, 10, -5);
}

void FVehicleManager::InitPhysXVehicle(PxPhysics* Physics, PxCooking* Cooking)
{
    CookPrimitiveMesh(Physics, Cooking);
    // Init Physical Material
    const float restitution = 0.2f;
    const float staticFriction = 0.5f;
    const float dynamicFriction = 0.5f;
    RoadMaterials = Physics->createMaterial(staticFriction, dynamicFriction, restitution);
    RoadTypes.mType = 0;
    VehicleMaterial = Physics->createMaterial(staticFriction, dynamicFriction, restitution);

    // Init Vehicle Environment
    PxInitVehicleSDK(*Physics);

    PxVec3 up(0, 0, 1);
    PxVec3 forward(1, 0, 0);
    PxVehicleSetBasisVectors(up, forward);

    PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);

    VehicleSimData = PxVehicleWheelsSimData::allocate(4);

    SurfaceTirePairs = PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(
        VehicleHelper::TIRE_TYPE_MAX, VehicleHelper::SURFACE_TYPES_MAX
    );
    SurfaceTirePairs->setup(VehicleHelper::TIRE_TYPE_MAX, VehicleHelper::SURFACE_TYPES_MAX, &RoadMaterials, &RoadTypes);
    for (uint8 i = 0; i < VehicleHelper::SURFACE_TYPES_MAX; ++i)
    {
        for (uint8 j = 0; j < VehicleHelper::TIRE_TYPE_MAX; ++j)
        {
            SurfaceTirePairs->setTypePairFriction(i, j, VehicleHelper::GetTireFrictionMultipliers(i, j));
        }
    }

    // Init WheelQuery
    const uint32 InitWheelCapacity = 16;
    WheelQueryResults = (PxWheelQueryResult*)malloc(sizeof(PxWheelQueryResult) * InitWheelCapacity);
    RaycastQueryResults = (PxRaycastQueryResult*)malloc(sizeof(PxRaycastQueryResult) * InitWheelCapacity);
    RaycastHits = (PxRaycastHit*)malloc(sizeof(PxRaycastHit) * InitWheelCapacity);
    WheelCapacity = InitWheelCapacity;

    // Init Mesh
    // ChassisMesh = Physics->create
}

void FVehicleManager::Shutdown()
{
    for (const auto& Vehicle: Vehicles)
    {
        switch (Vehicle->getVehicleType())
        {
        case PxVehicleTypes::eDRIVE4W:
            {
                PxVehicleDrive4W* veh=(PxVehicleDrive4W*)Vehicle;
                veh->free();
            }
            break;
        default:
            PX_ASSERT(false);
            break;
        }
    }

    // for (int i = 0; i < VehicleWheelsQueryResults.Num(); ++i)
    // {
    //     delete VehicleWheelsQueryResults[i];
    // }
    

    VehicleSimData->free();
    SurfaceTirePairs->release();
}

void FVehicleManager::CreateVehicle(PxPhysics* Physics, PxScene* Scene, const PxTransform& StartTransform)
{
    /** Create Simulation Data */
    PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(4);
    PxVehicleDriveSimData4W driveSimData;

    // Create Simulation Data: Chassis
    PxVehicleChassisData chassisData;
    
    VehicleHelper::AABB chassisAABB = ComputeMeshAABB(ChassisMesh);
    const PxVec3 chassisDims = chassisAABB.max - chassisAABB.min;
    const PxVec3 chassisCMOffset = PxVec3(0.0f, 0.f, 0.f);
    const PxVec3 chassisMOI(
        (chassisDims.y * chassisDims.y + chassisDims.z * chassisDims.z) * ChassisMass / 12.0f,
        (chassisDims.x * chassisDims.x + chassisDims.z * chassisDims.z) * ChassisMass / 12.0f,
        (chassisDims.x * chassisDims.x + chassisDims.y * chassisDims.y) * ChassisMass / 12.0f
    );

    chassisData.mMass = ChassisMass;
    chassisData.mMOI = chassisMOI;
    chassisData.mCMOffset = chassisCMOffset;

    // Create Simulation Data: Wheels
    PxVehicleWheelData wheels[4];
    for (int i = 0; i < 4; ++i)
    {
        VehicleHelper::AABB wheelAABB = ComputeMeshAABB(WheelMeshes[i]);
        wheels[i].mWidth = wheelAABB.max.x - wheelAABB.min.x;
        wheels[i].mRadius = std::max(wheelAABB.max.y, wheelAABB.max.z);
        wheels[i].mMOI = WheelMass * wheels[i].mRadius * wheels[i].mRadius / 2.0f;
        wheels[i].mMass = WheelMass;
    }
    
    wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxHandBrakeTorque = 0.0f;
    wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxHandBrakeTorque = 0.0f;
    wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxHandBrakeTorque = 4000.0f;
    wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxHandBrakeTorque = 4000.0f;
    
    wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxSteer = PxPi*0.3333f;
    wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxSteer = PxPi*0.3333f;
    wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxSteer = 0.0f;
    wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxSteer = 0.0f;

    // Create Simulation Data: Tires
    PxVehicleTireData tireData[4];
    tireData[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mType = VehicleHelper::TIRE_TYPE_WETS;
    tireData[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mType = VehicleHelper::TIRE_TYPE_WETS;
    tireData[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mType = VehicleHelper::TIRE_TYPE_WETS;
    tireData[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mType = VehicleHelper::TIRE_TYPE_WETS;

    // Create Simulation Data: Suspension Sprung
    float suspSprungMasses[4];
    PxVehicleComputeSprungMasses(4, WheelCentreOffsets, chassisCMOffset, ChassisMass, 2, suspSprungMasses);
    
    // Create Simulation Data: Suspension
    PxVehicleSuspensionData suspensions[4];
    for (int i = 0; i < 4; ++i)
    {
        suspensions[i].mMaxCompression = 0.3f;
        suspensions[i].mMaxDroop = 0.1f;
        suspensions[i].mSpringStrength = 35000.0f;
        suspensions[i].mSpringDamperRate = 4500.0f;
    }
    suspensions[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mSprungMass = suspSprungMasses[PxVehicleDrive4WWheelOrder::eFRONT_LEFT];
    suspensions[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mSprungMass = suspSprungMasses[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT];
    suspensions[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mSprungMass = suspSprungMasses[PxVehicleDrive4WWheelOrder::eREAR_LEFT];
    suspensions[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mSprungMass = suspSprungMasses[PxVehicleDrive4WWheelOrder::eREAR_RIGHT];

    const float camberAngleAtRest=0.0;
    suspensions[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mCamberAtRest = camberAngleAtRest;
    suspensions[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mCamberAtRest = -camberAngleAtRest;
    suspensions[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mCamberAtRest = camberAngleAtRest;
    suspensions[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mCamberAtRest = -camberAngleAtRest;
    
    const float camberAngleAtMaxDroop=0.001f;
    suspensions[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mCamberAtMaxDroop = camberAngleAtMaxDroop;
    suspensions[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mCamberAtMaxDroop = -camberAngleAtMaxDroop;
    suspensions[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mCamberAtMaxDroop = camberAngleAtMaxDroop;
    suspensions[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mCamberAtMaxDroop = -camberAngleAtMaxDroop;
    
    const float camberAngleAtMaxCompression=-0.001f;
    suspensions[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mCamberAtMaxCompression = camberAngleAtMaxCompression;
    suspensions[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mCamberAtMaxCompression = -camberAngleAtMaxCompression;
    suspensions[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mCamberAtMaxCompression = camberAngleAtMaxCompression;
    suspensions[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mCamberAtMaxCompression = -camberAngleAtMaxCompression;

    // Store Simulation Data to WheelsSimData
    for (int i = 0; i < 4; ++i)
    {
        PxVec3 suspensionTravelDirection = PxVec3(0, 0, -1);
        PxVec3 wheelCentreCMOffset = WheelCentreOffsets[i] - chassisCMOffset;
        PxVec3 suspensionForceAppCMOffset = PxVec3(wheelCentreCMOffset.x, wheelCentreCMOffset.y, 0.f);
        PxVec3 tireForceAppCMOffset = PxVec3(wheelCentreCMOffset.x, wheelCentreCMOffset.z, 0.f);

        wheelsSimData->setWheelData(i, wheels[i]);
        wheelsSimData->setTireData(i, tireData[i]);
        wheelsSimData->setSuspensionData(i, suspensions[i]);
        wheelsSimData->setSuspTravelDirection(i, suspensionTravelDirection);
        wheelsSimData->setWheelCentreOffset(i, wheelCentreCMOffset);
        wheelsSimData->setSuspForceAppPointOffset(i, suspensionForceAppCMOffset);
        wheelsSimData->setTireForceAppPointOffset(i, tireForceAppCMOffset);
    }
    wheelsSimData->setSubStepCount(5.0f, 3, 1);

    // Create Simulation Data: Differential
    PxVehicleDifferential4WData diff;
    diff.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;

    // Create Simulation Data: Engine
    PxVehicleEngineData engine;
    engine.mPeakTorque = 500.f;
    engine.mMaxOmega = 600.f;

    // Create Simulation Data: Gear
    PxVehicleGearsData gears;
    gears.mSwitchTime = 0.5f;

    // Create Simulation Data: Clutch
    PxVehicleClutchData clutch;
    clutch.mStrength = 10.f;

    // Create Simulation Data: Steering
    PxVehicleAckermannGeometryData ackermann;
    ackermann.mAccuracy = 1.0f;
    ackermann.mAxleSeparation = WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].x - WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT].x;
    ackermann.mFrontWidth = WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].y - WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].y;
    ackermann.mRearWidth = WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].y - WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT].y;
    
    // Store Simulation Data to DriveSimData
    driveSimData.setDiffData(diff);
    driveSimData.setEngineData(engine);
    driveSimData.setGearsData(gears);
    driveSimData.setClutchData(clutch);
    driveSimData.setAckermannGeometryData(ackermann);


    
    /** Create Vehicle Actor */
    PxRigidDynamic* vehicleActor = Physics->createRigidDynamic(PxTransform(PxIdentity));

    // wheel geometry, collider filter
    PxConvexMeshGeometry FLWheelGeom(WheelMeshes[0]);
    PxConvexMeshGeometry FRWheelGeom(WheelMeshes[1]);
    PxConvexMeshGeometry RLWheelGeom(WheelMeshes[2]);
    PxConvexMeshGeometry RRWheelGeom(WheelMeshes[3]);
    const PxGeometry* wheelGeometries[4] = {
        &FLWheelGeom, &FRWheelGeom, &RLWheelGeom, &RRWheelGeom
    };
    PxFilterData wheelCollFilterData;
    wheelCollFilterData.word0 = VehicleHelper::COLLISION_FLAG_WHEEL;
    wheelCollFilterData.word1 = VehicleHelper::COLLISION_FLAG_WHEEL_AGAINST;

    // chassis
    PxConvexMeshGeometry chassisGeom(ChassisMesh);
    PxFilterData chassisCollFilterData;
    chassisCollFilterData.word0 = VehicleHelper::COLLISION_FLAG_CHASSIS;
    chassisCollFilterData.word1 = VehicleHelper::COLLISION_FLAG_CHASSIS_AGAINST;

    // vehicle query filter
    PxFilterData vehicleQueryFilterData;
    vehicleQueryFilterData.word3 = 0x0000ffff;

    // setup Actor
    for (int i = 0; i < 4; ++i)
    {
        PxShape* wheelShape = PxRigidActorExt::createExclusiveShape(*vehicleActor, *wheelGeometries[i], *VehicleMaterial);
        wheelShape->setQueryFilterData(vehicleQueryFilterData);
        wheelShape->setSimulationFilterData(wheelCollFilterData);
        wheelShape->setLocalPose(PxTransform(PxIdentity));
    }
    PxShape* chassisShape = PxRigidActorExt::createExclusiveShape(*vehicleActor, chassisGeom, *VehicleMaterial);
    chassisShape->setQueryFilterData(vehicleQueryFilterData);
    chassisShape->setSimulationFilterData(chassisCollFilterData);
    chassisShape->setLocalPose(PxTransform(PxIdentity));

    vehicleActor->setMass(chassisData.mMass);
    vehicleActor->setMassSpaceInertiaTensor(chassisData.mMOI);
    vehicleActor->setCMassLocalPose(PxTransform(chassisData.mCMOffset, PxQuat(PxIdentity)));


    /** Create a Car */
    PxVehicleDrive4W* car = PxVehicleDrive4W::allocate(4);
    car->setup(Physics, vehicleActor, *wheelsSimData, driveSimData, 0);
    {
        PxSceneWriteLock scopedLock(*Scene);
        Scene->addActor(*vehicleActor);
    }

    car->mWheelsSimData.setWheelShapeMapping(0, 0);
    car->mWheelsSimData.setWheelShapeMapping(1, 1);
    car->mWheelsSimData.setWheelShapeMapping(2, 2);
    car->mWheelsSimData.setWheelShapeMapping(3, 3);
    car->mWheelsSimData.setSceneQueryFilterData(0, vehicleQueryFilterData);
    car->mWheelsSimData.setSceneQueryFilterData(1, vehicleQueryFilterData);
    car->mWheelsSimData.setSceneQueryFilterData(2, vehicleQueryFilterData);
    car->mWheelsSimData.setSceneQueryFilterData(3, vehicleQueryFilterData);
    car->setToRestState();
    car->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    {
        PxSceneWriteLock scopedLock(*Scene);
        vehicleActor->setGlobalPose(StartTransform);
    }

    Vehicles.Add(car);
    PxVehicleWheelQueryResult queryResult;
    queryResult.nbWheelQueryResults = 4;
    if (WheelCount + 4 > WheelCapacity)
    {
        ReallocWheelQueryResults();
    }
    queryResult.wheelQueryResults = WheelQueryResults + WheelCount;
    WheelCount += 4;
    VehicleWheelsQueryResults.Add(queryResult);

    
    /** Release Resource */
    wheelsSimData->free();
}

void FVehicleManager::Update(const float deltaTime, PxScene* Scene)
{
    UpdateDigitalInput();
    UpdateParameterTargetVehicle(deltaTime);
    PxVehicleUpdates(deltaTime, Scene->getGravity(), *SurfaceTirePairs, Vehicles.Num(), Vehicles.GetData(), VehicleWheelsQueryResults.GetData());
}

void FVehicleManager::SuspensionRaycasts(PxScene* scene)
{
    if (RaycastBatchQuery == nullptr)
    {
        PxBatchQueryDesc desc(WheelCapacity, 0, 0);
        desc.queryMemory.userRaycastResultBuffer = RaycastQueryResults;
        desc.queryMemory.userRaycastTouchBuffer = RaycastHits;
        desc.queryMemory.raycastTouchBufferSize = WheelCapacity;
        desc.preFilterShader = SampleVehicleWheelRaycastPreFilter;
        RaycastBatchQuery = scene->createBatchQuery(desc);
    }
    PxSceneReadLock scopedLock(*scene);
    PxVehicleSuspensionRaycasts(RaycastBatchQuery, Vehicles.Num(), Vehicles.GetData(), WheelCapacity, RaycastQueryResults); 
}

VehicleHelper::AABB FVehicleManager::ComputeMeshAABB(const PxConvexMesh* mesh)
{
    const PxU32 numChassisVerts = mesh->getNbVertices();
    const PxVec3* chassisVerts = mesh->getVertices();
    PxVec3 minVector(PX_MAX_F32,PX_MAX_F32,PX_MAX_F32);
    PxVec3 maxVector(-PX_MAX_F32,-PX_MAX_F32,-PX_MAX_F32);
    for(PxU32 i=0;i<numChassisVerts;i++)
    {
        minVector.x=PxMin(minVector.x,chassisVerts[i].x);
        minVector.y=PxMin(minVector.y,chassisVerts[i].y);
        minVector.z=PxMin(minVector.z,chassisVerts[i].z);
        maxVector.x=PxMax(maxVector.x,chassisVerts[i].x);
        maxVector.y=PxMax(maxVector.y,chassisVerts[i].y);
        maxVector.z=PxMax(maxVector.z,chassisVerts[i].z);
    }
    return { minVector,maxVector };
}

void FVehicleManager::ReallocWheelQueryResults()
{
    WheelCapacity *= 2;
    WheelQueryResults = (PxWheelQueryResult*)realloc((void*)WheelQueryResults, sizeof(PxWheelQueryResult) * WheelCapacity);
    RaycastQueryResults = (PxRaycastQueryResult*)realloc((void*)RaycastQueryResults, sizeof(PxRaycastQueryResult) * WheelCapacity);
    RaycastHits = (PxRaycastHit*)realloc((void*)RaycastHits, sizeof(PxRaycastHit) * WheelCapacity);

    for (int i = 0; i < VehicleWheelsQueryResults.Num(); ++i)
    {
        VehicleWheelsQueryResults[i].wheelQueryResults = WheelQueryResults + i * 4;
    }
}

void FVehicleManager::CookPrimitiveMesh(PxPhysics* Physics, PxCooking* Cooking)
{
    {
        static PxVec3 ChassisVertices[] =
        {
            PxVec3(-20.f, -10.f, -5.f),
            PxVec3(-20.f, -10.f,  5.f),
            PxVec3(-20.f,  10.f, -5.f),
            PxVec3(-20.f,  10.f,  5.f),
            PxVec3( 20.f, -10.f, -5.f),
            PxVec3( 20.f, -10.f,  5.f),
            PxVec3( 20.f,  10.f, -5.f),
            PxVec3( 20.f,  10.f,  5.f)
        };
    
        PxConvexMeshDesc convexDesc;
        convexDesc.points.count     = 8;
        convexDesc.points.stride    = sizeof(PxVec3);
        convexDesc.points.data      = ChassisVertices;
        convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
    
        PxDefaultMemoryOutputStream writeBuffer;
        if (!Cooking->cookConvexMesh(convexDesc, writeBuffer)) {
            return;
        }

        PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
        ChassisMesh = Physics->createConvexMesh(readBuffer);
    }

    {
        static PxVec3 WheelVertices[] = {
            PxVec3(-5.f, -5.f, -5.f),
            PxVec3(-5.f, -5.f,  5.f),
            PxVec3(-5.f,  5.f, -5.f),
            PxVec3(-5.f,  5.f,  5.f),
            PxVec3( 5.f, -5.f, -5.f),
            PxVec3( 5.f, -5.f,  5.f),
            PxVec3( 5.f,  5.f, -5.f),
            PxVec3( 5.f,  5.f,  5.f)
        };

    
        PxConvexMeshDesc convexDesc;
        convexDesc.points.count     = 8;
        convexDesc.points.stride    = sizeof(PxVec3);
        convexDesc.points.data      = WheelVertices;
        convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
    
        PxDefaultMemoryOutputStream writeBuffer;
        if (!Cooking->cookConvexMesh(convexDesc, writeBuffer)) {
            return;
        }

        for (int i = 0; i < 4; ++i)
        {
            PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
            WheelMeshes[i] = Physics->createConvexMesh(readBuffer);
        }
    }
}

PxQueryHitType::Enum FVehicleManager::SampleVehicleWheelRaycastPreFilter(	
    PxFilterData filterData0, 
    PxFilterData filterData1,
    const void* constantBlock, PxU32 constantBlockSize,
    PxHitFlags& queryFlags
)
{
    //filterData0 is the vehicle suspension raycast.
    //filterData1 is the shape potentially hit by the raycast.
    PX_UNUSED(queryFlags);
    PX_UNUSED(constantBlockSize);
    PX_UNUSED(constantBlock);
    PX_UNUSED(filterData0);
    return ((0 == (filterData1.word3 & 0xffff0000)) ? PxQueryHitType::eNONE : PxQueryHitType::eBLOCK);
}

void FVehicleManager::UpdateParameterTargetVehicle(float deltaTime)
{
    if (TargetVehicleIndex < 0 || Vehicles.Num() <= TargetVehicleIndex)
        return;
    PxVehicleDrive4W* focusVehicle = static_cast<PxVehicleDrive4W*>(Vehicles[TargetVehicleIndex]);
    PxVehicleDriveDynData* driveDynData = &focusVehicle->mDriveDynData;

    PxVehicleDrive4WRawInputData carRawInputs;
    carRawInputs.setDigitalAccel(Inputs.bAccelKey);
    carRawInputs.setDigitalBrake(Inputs.bBrakeKey);
    carRawInputs.setDigitalHandbrake(Inputs.bHandBrakeKey);
    carRawInputs.setDigitalSteerLeft(Inputs.bSteerLeftKey);
    carRawInputs.setDigitalSteerRight(Inputs.bSteerRightKey);
    carRawInputs.setGearUp(Inputs.bGearUpKey);
    carRawInputs.setGearDown(Inputs.bGearDownKey);

    const bool isAir = PxVehicleIsInAir(VehicleWheelsQueryResults[0]);
    PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(
        KeySmoothingData,
        SteerVsForwardSpeedTable,
        carRawInputs,
        deltaTime,
        false,
        *focusVehicle
    );
    
    
}

void FVehicleManager::UpdateDigitalInput()
{
    if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000))
    {
        Inputs.bSteerLeftKey = !(!(GetAsyncKeyState('A') & 0x8000));
        Inputs.bSteerRightKey = !(!(GetAsyncKeyState('D') & 0x8000));
        Inputs.bAccelKey = !(!(GetAsyncKeyState('W') & 0x8000));
        Inputs.bBrakeKey = !(!(GetAsyncKeyState('S') & 0x8000));
        Inputs.bHandBrakeKey = !(!(GetAsyncKeyState('X') & 0x8000));
        Inputs.bGearDownKey = !(!(GetAsyncKeyState('Q') & 0x8000));
        Inputs.bGearUpKey = !(!(GetAsyncKeyState('E') & 0x8000));
    }
}
