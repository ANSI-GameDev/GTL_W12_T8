#pragma once
#include <PxFoundation.h>
#include <extensions/PxDefaultAllocator.h>
#include <extensions/PxDefaultCpuDispatcher.h>
#include <extensions/PxDefaultErrorCallback.h>
#include <pvd/PxPvd.h>
#include <vehicle/PxVehicleUpdate.h>
#include <vehicle/PxVehicleWheels.h>

#include "VehicleManager.h"
#include "Container/Array.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/ConstraintInstance.h"

#define PX_RELEASE(x) if ((x)) { (x)->release(); (x) = nullptr; }
#define PVD_HOST "127.0.0.1"
#define MAX_NUM_ACTOR_SHAPES 128

class UWorld;

using namespace physx;  // NOLINT(clang-diagnostic-header-hygiene)

class physx::PxDefaultCpuDispatcher;
class physx::PxPvd;
class physx::PxFoundation;

class FPhysScene
{
public:
    TArray<FBodyInstance*> BodyInstances;
    TArray<FConstraintInstance*> Constraints;

    void SetOwningWorld(UWorld* InOwningWorld);
    UWorld* GetOwningWorld() const;
    
    virtual void TickPhysScene(float DeltaTime);
    virtual void WaitPhysScenes();
    virtual void InitPhysX();
    void Simulate(float DeltaTime);
    //물리 초기화 및 종료 구현
    // FCriticalSection SceneLock;
    // FPhysSubstepTasks SubStepping; //고정된 타임스텝을 유지하기 위해 작은 시간 간격으로 여러번 물리업데이트 수행
    
    // TArray<FCollisionNotifyInfo> PendingCollisionNotifies;
    UObject* Owner;
    
    PxDefaultAllocator      gAllocator;
    PxDefaultErrorCallback  gErrorCallback;
    PxFoundation* gFoundation = nullptr;
    PxPvd* gPvd = nullptr;
    PxPhysics* gPhysics = nullptr;
    PxScene* gScene = nullptr;
    PxMaterial* gMaterial = nullptr;
    PxDefaultCpuDispatcher* gDispatcher = nullptr;
    PxCooking* gCooking = nullptr;

    FVehicleManager* VehicleManager = nullptr;
};
