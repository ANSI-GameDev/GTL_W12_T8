#pragma once
#include <PxPhysicsAPI.h>

#define PX_RELEASE(x) if ((x)) { (x)->release(); (x) = nullptr; }
#define PVD_HOST "127.0.0.1"
#define MAX_NUM_ACTOR_SHAPES 128

class UObject;
class UWorld;

using namespace physx;

class FPhysScene
{
public:
    FPhysScene();
    ~FPhysScene();

    void InitPhysX();
    void CleanupPhysX();
    void SimulatePhysX();

    void SetOwningWorld(UWorld* InOwningWorld);
    UWorld* GetOwningWorld() const;


protected:
    UObject* Owner;

    //TArray<

    PxDefaultAllocator      gAllocator;
    PxDefaultErrorCallback  gErrorCallback;
    PxFoundation* gFoundation = nullptr;
    PxPvd* gPvd = nullptr;
    PxPhysics* gPhysics = nullptr;
    PxScene* gScene = nullptr;
    PxMaterial* gMaterial = nullptr;
    PxDefaultCpuDispatcher* gDispatcher = nullptr;


};

