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

        FVector CMOffset = Vehicle->getRigidDynamicActor()->getCMassLocalPose().p;
        FVector WheelOffset = GetWheelPosition(i);
        FVector offset = FMatrix::CreateRotationMatrix(GetActorRotation()).TransformPosition(CMOffset + WheelOffset);
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
    
    VehicleHelper::CreateVehicleData data = VehicleHelper::CreateVehicleData();
    data.initPosition = GetActorLocation().ToPxVec3();
    data.chassisAABB.min = ModifiedChassisAABBMin.ToPxVec3();
    data.chassisAABB.max = ModifiedChassisAABBMax.ToPxVec3();
    data.chassisCMOffset = ( (ModifiedChassisAABBMax + ModifiedChassisAABBMin).ToPxVec3() / 2.0f );
    for (int i = 0; i < 4; ++i)
    {
        data.wheelRadius[i] = GetWheelRadius(i);
        data.wheelWidth[i] = GetWheelWidth(i);
        data.wheelCentreOffsets[i] = GetWheelPosition(i).ToPxVec3();
    }
    FPhysScene* phyScene = GetWorld()->GetPhysicsScene();
    PxVehicleDrive4W* vehicle = phyScene->VehicleManager->CreateVehicle(data);
    
    RemoveVehicle();
    Vehicle = vehicle;

    FBodyInstance* vehicleBody = new FBodyInstance();
    vehicleBody->InitBody(vehicle->getRigidDynamicActor(), StaticMeshComponent->GetComponentTransform().Translation, GetWorld()->GetPhysicsScene());
    StaticMeshComponent->SetBodyInstance(vehicleBody);
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

float AVehicle::GetWheelRpm()
{
    float sum = 0.f;
    for (int i = 0; i < 4; ++i)
    {
        sum += Vehicle->mWheelsDynData.getWheelRotationSpeed(i);
    }
    float avg = sum / 4;
    float rpm = avg * 60;
    return rpm;
}

