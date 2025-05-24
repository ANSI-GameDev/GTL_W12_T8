#include "PhysScene.h"

#include "UObject/Casts.h"
#include "World/World.h"

#include <PxPhysicsAPI.h>

using namespace physx;

void FPhysScene::TickPhysScene(float DeltaTime)
{
    Simulate(DeltaTime);
}

void FPhysScene::WaitPhysScenes()
{
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
    gDispatcher = PxDefaultCpuDispatcherCreate(4);
    
    //staticFriction: 정지 마찰력 (0.5)
    // dynamicFriction: 운동 마찰력 (0.5)
    // restitution: 반발 계수 (0.6) → 충돌 후 얼마나 튕길지를 결정
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
    
    PxSceneDesc SceneDesc(gPhysics->getTolerancesScale());
    SceneDesc.gravity = PxVec3(0.0f, 0.0f, -9.81f);
    SceneDesc.cpuDispatcher = gDispatcher;
    SceneDesc.filterShader = PxDefaultSimulationFilterShader;
    SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    SceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    SceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
    gScene = gPhysics->createScene(SceneDesc);

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

    //임시로 박스만들기
    PxVec3 pos = PxVec3(0, 0, 10);
    PxVec3 halfExtents = PxVec3(1, 1, 1);
    
    PxRigidBody* rigidBody = nullptr;
    
    PxTransform pose(pos);
    rigidBody = gPhysics->createRigidDynamic(pose);
    PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *gMaterial);
    rigidBody->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*rigidBody, 10.0f);
    gScene->addActor(*rigidBody);
    // obj.UpdateFromPhysics();

    PxRigidStatic* rigidStatic = nullptr;
    PxPlane plane = PxPlane(0, 0, 1, 0);
    
    rigidStatic = PxCreatePlane(*gPhysics, plane, *gMaterial);
    gScene->addActor(*rigidStatic);

}

void FPhysScene::Simulate(float DeltaTime)
{
    gScene->simulate(DeltaTime);
    gScene->fetchResults(true);
    for (FBodyInstance& BodyInstance : BodyInstances)
    {
        if (BodyInstance.bSimulatePhysics)
        {
            BodyInstance.UpdatePhysics();
        }
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
