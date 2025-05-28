#include "Vehicle.h"

#include <PxPhysics.h>
#include <extensions/PxRigidBodyExt.h>


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
    PxVehicleDrive4W* vehicle = phyScene->VehicleManager->CreateVehicle(phyScene->gPhysics);
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
    FPhysScene* Scene = GetWorld()->GetPhysicsScene();
    PxRigidDynamic* chassisActor = Vehicle->getRigidDynamicActor();

    PxVec3 offset = ((ModifiedChassisAABBMax + ModifiedChassisAABBMin) / 2).ToPxVec3();
    PxVec3 halfExtents = ((ModifiedChassisAABBMax - ModifiedChassisAABBMin) / 2).ToPxVec3(); // 예: 4x2x2 box
    PxBoxGeometry box(halfExtents);

    const PxMaterial* material = Scene->VehicleManager->GetVehicleMaterial();

    PxShape* oldShape = nullptr;
    chassisActor->getShapes(&oldShape, 1);
    chassisActor->detachShape(*oldShape);
    oldShape->release();

    PxShape* shape = Scene->gPhysics->createShape(box, *material);
    shape->setLocalPose(PxTransform(offset));
    chassisActor->attachShape(*shape);
    shape->release();

    // 질량/관성 업데이트
    PxRigidBodyExt::updateMassAndInertia(*chassisActor, ChassisMass); 
}
