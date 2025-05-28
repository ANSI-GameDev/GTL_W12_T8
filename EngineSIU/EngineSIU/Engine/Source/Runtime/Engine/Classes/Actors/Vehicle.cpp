#include "Vehicle.h"

#include <PxPhysics.h>
#include <PxScene.h>
#include <PxSceneLock.h>
#include <extensions/PxRigidBodyExt.h>
#include <vehicle/PxVehicleUtilSetup.h>


#include "PhysScene.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/EditorEngine.h"
#include "World/World.h"

AVehicle::AVehicle()
{
    StaticMeshComponent->SetStaticMesh(UAssetManager::Get().GetStaticMesh(FName(L"Contents/Asset/ae86.fbx")));
    Targeted = false;
}

void AVehicle::InitVehicle()
{
    SetActorLocation(FVector(0, 0, 10));
    SetActorTickInEditor(true);

    FPhysScene* phyScene = GetWorld()->GetPhysicsScene();
    PxVehicleDrive4W* vehicle = phyScene->VehicleManager->CreateVehicle();
    Vehicle = vehicle;
    
    FBodyInstance* vehicleBody = new FBodyInstance();
    vehicleBody->InitBody(vehicle->getRigidDynamicActor(), StaticMeshComponent->GetComponentTransform().Translation, GetWorld()->GetPhysicsScene());
    StaticMeshComponent->SetBodyInstance(vehicleBody);

    PxVehicleDriveSimData4W simData = Vehicle->mDriveSimData;
    EngineTorqueMax = simData.getEngineData().mPeakTorque;
    EngineRotSpeedMax = simData.getEngineData().mMaxOmega;
    GearSwitchTime = simData.getGearsData().mSwitchTime;
    ChassisMass = Vehicle->getRigidDynamicActor()->getMass();
    
    ModifiedChassisAABBMin = FVector(-20, -10, -5);
    ModifiedChassisAABBMax = FVector(20, 10, 5);

    StaticMeshComponent->AABB = FBoundingBox(ModifiedChassisAABBMin, ModifiedChassisAABBMax);
}

void AVehicle::Tick(float DeltaTime)
{
    AStaticMeshActor::Tick(DeltaTime);

    FVehicleManager* VehicleManager = GetWorld()->GetPhysicsScene()->VehicleManager;
    if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
    {
        if (EditorEngine->GetSelectedActor() == this)
        {
            if (Targeted)
                VehicleManager->TargetVehicleIndex = VehicleManager->Vehicles.Find(Vehicle);

            StaticMeshComponent->AABB.MinLocation = ModifiedChassisAABBMin;
            StaticMeshComponent->AABB.MaxLocation = ModifiedChassisAABBMax;
            UpdateProperties();
        }
        else
        {
            if (0 <= VehicleManager->TargetVehicleIndex && VehicleManager->TargetVehicleIndex < VehicleManager->Vehicles.Num())
                Targeted = (VehicleManager->Vehicles[VehicleManager->TargetVehicleIndex] == Vehicle);
            else
                Targeted = false;
        }
    }

    // TODO: Tick에서 그냥 RenderPass에 던지면 됨??
    GEngineLoop.Renderer.PrimitiveDrawBatch->AddOBBToBatch(
        FBoundingBox(ModifiedChassisAABBMin, ModifiedChassisAABBMax),
        GetActorLocation(),
        FMatrix::CreateRotationMatrix(GetActorRotation())
    );
    for (int i = 0; i < 4; ++i)
    {
        float radius = GetWheelRadius(i);
        float width = GetWheelWidth(i);
        FBoundingBox wheelBox(
            FVector(-radius / 2.f, -width / 2.f, -radius / 2.f),
            FVector(radius / 2.f, width / 2.f, radius / 2.f)
        );
        
        PxShape* wheelShape;
        Vehicle->getRigidDynamicActor()->getShapes(&wheelShape, 1, i);
        PxTransform localPose = wheelShape->getLocalPose();
        PxQuat q = localPose.q;
        FRotator rotator(FQuat(q.x, q.y, q.z, q.w));
        rotator.Pitch = 0.f;
        
        FVector offset = FMatrix::CreateRotationMatrix(GetActorRotation()).TransformPosition(GetWheelPosition(i));
        FMatrix matrix = FMatrix::CreateRotationMatrix(rotator);
        matrix *= FMatrix::CreateRotationMatrix(GetActorRotation());
        GEngineLoop.Renderer.PrimitiveDrawBatch->AddOBBToBatch(
            wheelBox,
            GetActorLocation() + offset,
            matrix
        );
    }
}

void AVehicle::Destroyed()
{
    AStaticMeshActor::Destroyed();
    RemoveVehicle();
}

void AVehicle::RemoveVehicle()
{
    FPhysScene* phyScene = GetWorld()->GetPhysicsScene();
    phyScene->gScene->removeActor(*Vehicle->getRigidDynamicActor());
    phyScene->VehicleManager->RemoveVehicle(Vehicle);
    Vehicle->free();
}

FVector AVehicle::GetWheelPosition(int index)
{
    return GetWheelSimData().getWheelCentreOffset(index);
}

float AVehicle::GetWheelRadius(int index)
{
    return GetWheelSimData().getWheelData(index).mRadius;
}

float AVehicle::GetWheelWidth(int index)
{
    return GetWheelSimData().getWheelData(index).mWidth;
}

void AVehicle::SetWheelPosition(int index, FVector position)
{
    GetWheelSimData().setWheelCentreOffset(index, position.ToPxVec3());
}

void AVehicle::SetWheelRadius(int index, float radius)
{
    PxVehicleWheelData wheelData = GetWheelSimData().getWheelData(index);
    wheelData.mRadius = radius;
    wheelData.mMOI = wheelData.mMass * radius * radius / 2.0f;
    GetWheelSimData().setWheelData(index, wheelData);
}

void AVehicle::SetWheelWidth(int index, float width)
{
    PxVehicleWheelData wheelData = GetWheelSimData().getWheelData(index);
    wheelData.mWidth = width;
    GetWheelSimData().setWheelData(index, wheelData);
}

uint32 AVehicle::GetCurrentGearNum()
{
    return Vehicle->mDriveDynData.getCurrentGear();
}

physx::PxVehicleWheelsSimData& AVehicle::GetWheelSimData()
{
    return Vehicle->mWheelsSimData;
}

void AVehicle::UpdateProperties()
{
    {
        PxVehicleEngineData engine = Vehicle->mDriveSimData.getEngineData();
        engine.mPeakTorque = EngineTorqueMax;
        engine.mMaxOmega = EngineRotSpeedMax;
        Vehicle->mDriveSimData.setEngineData(engine);
    }
    {
        PxVehicleGearsData gears = Vehicle->mDriveSimData.getGearsData();
        gears.mSwitchTime = GearSwitchTime;
        Vehicle->mDriveSimData.setGearsData(gears); 
    }
}

void AVehicle::ApplyModifiedChassis()
{
    FVector dim = ModifiedChassisAABBMax - ModifiedChassisAABBMin;
    if (dim.X <= 0 || dim.Y <= 0 || dim.Z <= 0)
    {
        UE_LOG(ELogLevel::Error, "Can not resize Chassis");
        return;
    }
    FPhysScene* Scene = GetWorld()->GetPhysicsScene();
    PxRigidDynamic* chassisActor = Vehicle->getRigidDynamicActor();

    PxVec3 offset = ((ModifiedChassisAABBMax + ModifiedChassisAABBMin) / 2).ToPxVec3();
    PxVec3 halfExtents = ((ModifiedChassisAABBMax - ModifiedChassisAABBMin) / 2).ToPxVec3();
    PxBoxGeometry box(halfExtents);
    PxVec3 wheelCentreCMOffset[4];
    for (int i = 0; i < 4; ++i)
    {
        wheelCentreCMOffset[i] = GetWheelPosition(i).ToPxVec3() - offset;
    }
    float suspSprungMasses[4];
    PxVehicleComputeSprungMasses(4, wheelCentreCMOffset, offset, ChassisMass, 2, suspSprungMasses);
    for (int i = 0; i < 4; i++)
    {
        PxVec3 suspensionForceAppCMOffset = PxVec3(wheelCentreCMOffset[i].x, wheelCentreCMOffset[i].y, 0.f);
        PxVec3 tireForceAppCMOffset = PxVec3(wheelCentreCMOffset[i].x, wheelCentreCMOffset[i].z, 0.f);

        // Vehicle->mWheelsSimData.setSuspensionData(i, suspensions[i]);
        PxVehicleSuspensionData suspData = Vehicle->mWheelsSimData.getSuspensionData(i);
        suspData.mSprungMass = suspSprungMasses[i];
        Vehicle->mWheelsSimData.setSuspensionData(i, suspData);
        Vehicle->mWheelsSimData.setWheelCentreOffset(i, wheelCentreCMOffset[i]);
        Vehicle->mWheelsSimData.setSuspForceAppPointOffset(i, suspensionForceAppCMOffset);
        Vehicle->mWheelsSimData.setTireForceAppPointOffset(i, tireForceAppCMOffset);
    }

    const PxMaterial* material = Scene->VehicleManager->GetVehicleMaterial();

    PxShape* oldShape = nullptr;
    chassisActor->getShapes(&oldShape, 1, 4);
    chassisActor->detachShape(*oldShape);
    // oldShape->release();

    // new shape filters
    PxFilterData chassisCollFilterData;
    chassisCollFilterData.word0 = VehicleHelper::COLLISION_FLAG_CHASSIS;
    chassisCollFilterData.word1 = VehicleHelper::COLLISION_FLAG_CHASSIS_AGAINST;
    PxFilterData vehicleQueryFilterData;
    vehicleQueryFilterData.word3 = 0x0000ffff;
    PxShape* shape = Scene->gPhysics->createShape(box, *material);
    shape->setQueryFilterData(vehicleQueryFilterData);
    shape->setSimulationFilterData(chassisCollFilterData);
    shape->setLocalPose(PxTransform(offset));
    
    chassisActor->attachShape(*shape);
    shape->release();

    // MOI
    const PxVec3 chassisDims = dim.ToPxVec3();
    const PxVec3 chassisMOI(
        (chassisDims.y * chassisDims.y + chassisDims.z * chassisDims.z) * ChassisMass / 12.0f,
        (chassisDims.x * chassisDims.x + chassisDims.z * chassisDims.z) * ChassisMass / 12.0f,
        (chassisDims.x * chassisDims.x + chassisDims.y * chassisDims.y) * ChassisMass / 12.0f
    );
    chassisActor->setMassSpaceInertiaTensor(chassisMOI);
    
    PxRigidBodyExt::updateMassAndInertia(*chassisActor, ChassisMass, &offset); 
}

void AVehicle::UpdateByModified()
{
    PxVehicleWheelsSimData* wheelSimData = &Vehicle->mWheelsSimData;
    PxVehicleDriveSimData4W driveSimData = Vehicle->mDriveSimData;
    // wheelSimData->
    // Vehicle->setup()
}
void AVehicle::ResetPosition()
{
    {
        float minz = 0.f;
        for (int i = 0; i < 4; ++i)
        {
            minz = std::min(GetWheelPosition(i).Z - GetWheelRadius(i), minz);
        }
        minz = std::min(minz, ModifiedChassisAABBMin.Z);
        FPhysScene* phyScene = GetWorld()->GetPhysicsScene();
        PxSceneWriteLock scopedLock(*phyScene->gScene);
        Vehicle->getRigidDynamicActor()->setGlobalPose(PxTransform(0.f, 0.f, -minz + 10.f));
        Vehicle->getRigidDynamicActor()->setLinearVelocity(PxVec3(0.f, 0.f, 0.f));
        Vehicle->getRigidDynamicActor()->setAngularVelocity(PxVec3(0.f, 0.f, 0.f));
    }
}

