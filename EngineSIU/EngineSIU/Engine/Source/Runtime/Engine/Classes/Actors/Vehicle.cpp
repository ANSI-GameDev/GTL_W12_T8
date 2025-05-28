#include "Vehicle.h"


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

