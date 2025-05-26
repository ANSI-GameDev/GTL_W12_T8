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
    PxInitExtensions(*gPhysics, gPvd);
    
    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    
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


    //Body1
    // PxRigidBody* RBody2= nullptr;
    //
    // PxVec3 Pos2 = PxVec3(5, 0, 5);
    // PxTransform RPos2(Pos2);
    // RBody2 = gPhysics->createRigidDynamic(RPos2);
    //
    // PxShape* RShape2 =gPhysics->createShape(PxSphereGeometry(1.f), *gMaterial);
    // RBody2->attachShape(*RShape2);
    
    //Body2
    // PxVec3 pos = PxVec3(0, 0, 10);
    // PxVec3 halfExtents = PxVec3(1, 1, 1);
    // PxRigidBody* rigidBody = nullptr;
    // PxTransform pose(pos);
    // rigidBody = gPhysics->createRigidDynamic(pose);
    //
    // PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *gMaterial);
    // PxTransform ShapePose = PxTransform(PxVec3(5, 5, 5));
    // shape->setLocalPose(ShapePose);
    // rigidBody->attachShape(*shape);
    //
    // PxShape* Shape2 = gPhysics->createShape(PxSphereGeometry(0.5f), *gMaterial);
    // PxTransform Shape2Pose = PxTransform(PxVec3(0, 0, 0));
    // Shape2->setLocalPose(Shape2Pose);
    // rigidBody->attachShape(*Shape2);
    //
    // if (!rigidBody->getScene())
    // {
    //     gScene->addActor(*rigidBody);
    // }
    // if (!RBody2->getScene())
    // {
    //     gScene->addActor(*RBody2);
    // }

    //addActor하고 Joint생성
       
    PxRigidStatic* rigidStatic = nullptr;
    PxPlane plane = PxPlane(0, 0, 1, 0);
    
    // PxD6Joint* Joint = PxD6JointCreate(*gPhysics, RBody2, RPos2, rigidBody, pose);
    // Joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE); //회전 고정
    // Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE); //회전 고정
    // Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE); //회전 고정
    // Joint->setMotion(PxD6Axis::eX, PxD6Motion::eFREE); //X축 자유이동
    // Joint->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(0, 1000, FLT_MAX, true));
    // Joint->setLinearLimit(PxD6Axis::eX, PxJointLinearLimitPair(0.f, 1.0f, PxSpring(100.0f, 10.0f))); //PxJointLinearLimitPair
    //attachShape후에 호출 (걍 마지막에 호출
    // PxRigidBodyExt::updateMassAndInertia(*rigidBody, 10.0f);
    // obj.UpdateFromPhysics();

    
    rigidStatic = PxCreatePlane(*gPhysics, plane, *gMaterial);
    gScene->addActor(*rigidStatic);
}

void FPhysScene::Simulate(float DeltaTime)
{
    gScene->simulate(DeltaTime);
    gScene->fetchResults(true);
    for (FBodyInstance* BodyInstance : BodyInstances)
    {
        if (BodyInstance->bSimulatePhysics)
        {
            BodyInstance->UpdatePhysics();
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
