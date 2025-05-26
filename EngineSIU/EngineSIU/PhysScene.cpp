#include "PhysScene.h"
#include "Engine/World/World.h"
#include "UObject/Casts.h"

FPhysScene::FPhysScene()
    : Owner(nullptr)
{
    InitPhysX();
}

FPhysScene::~FPhysScene()
{
    CleanupPhysX();
}

void FPhysScene::InitPhysX()
{
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

    /* Visual Debugger Transport 설정 */
    gPvd = PxCreatePvd(*gFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL); // Debug | Profile | Memory
    if (!gPvd->isConnected())
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to Connect PVD! Make sure to Execute"));
    }

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
    gDispatcher = PxDefaultCpuDispatcherCreate(2);

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0, -9.81f, 0);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

    gScene = gPhysics->createScene(sceneDesc);

    /* Visual Debugger 활성화
     * 1) 물리제약조건 2) 충돌 지점 3) SceneQuery(Raycast, Sweep, Overlap) 정보를 PVD로 전송
     */
    PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
    if (pvdClient)
    {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
}

void FPhysScene::CleanupPhysX()
{
    PX_RELEASE(gScene);
    PX_RELEASE(gDispatcher);
    PX_RELEASE(gPhysics);
    if (gPvd && gPvd->isConnected())
    {
        PxPvdTransport* transport = gPvd->getTransport();
        gPvd->release();
        gPvd = nullptr;
        PX_RELEASE(transport);
    }
    PX_RELEASE(gFoundation);
    
}

void FPhysScene::SimulatePhysX()
{
    if (gScene)
    {
        gScene->simulate(1.0f / 60.0f);
        gScene->fetchResults(true);
    }

    // Ragdoll 본들 위치 업데이트
}

void FPhysScene::SetOwningWorld(UWorld* InOwningWorld)
{
    Owner = InOwningWorld;
}

UWorld* FPhysScene::GetOwningWorld() const
{
    return Cast<UWorld>(Owner);
}
