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
    SceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
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


    PxVec3 staticPos(5.0f, 0.0f, 15.0f);
    PxTransform staticTransform(staticPos);
    PxRigidStatic* staticActor = gPhysics->createRigidStatic(staticTransform);

    PxShape* sphereShape = gPhysics->createShape(PxSphereGeometry(1.0f), *gMaterial);
    staticActor->attachShape(*sphereShape);
    gScene->addActor(*staticActor);

    // --- Dynamic Body 생성 (이전 rigidBody 대신) ---
    PxVec3 dynPos(6.5f, 0.0f, 10.f);
    PxTransform dynamicTransform(dynPos);
    PxRigidDynamic* dynamicActor = gPhysics->createRigidDynamic(dynamicTransform);

    // 박스 충돌체
    PxBoxGeometry boxGeom(PxVec3(1.0f, 1.0f, 1.0f));
    PxShape* boxShape = gPhysics->createShape(boxGeom, *gMaterial);
    boxShape->setLocalPose(PxTransform(PxVec3(0.0f)));
    dynamicActor->attachShape(*boxShape);
    PxRigidBodyExt::updateMassAndInertia(*dynamicActor, 10.0f);
    gScene->addActor(*dynamicActor);

    // --- Joint 생성 --- //
    PxTransform worldJointPose = staticActor->getGlobalPose();
    PxVec3 jointDir = (dynPos - staticPos).getNormalized();
    PxQuat alignToX = PxShortestRotation(PxVec3(1, 0, 0), jointDir); // 이 벡터가 로컬 X축이 되도록 회전 만들기

    PxTransform localFrameStatic = PxTransform(PxIdentity) * PxTransform(alignToX);
    PxTransform localFrameDynamic = dynamicActor->getGlobalPose().getInverse() * worldJointPose * PxTransform(alignToX);

    PxD6Joint* joint = PxD6JointCreate(*gPhysics,staticActor, localFrameStatic,dynamicActor, localFrameDynamic);
    joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

    // 1) 회전 자유도 설정 (진자 움직임)
    joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);
    joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
    joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);

    joint->setSwingLimit(PxJointLimitCone(FMath::DegreesToRadians(60.f), FMath::DegreesToRadians(60.f), { 50.f, 7.f }));

    // X축만 리미트 모드로 설정
    joint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);  // X축만 스프링/리미트
    joint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
    joint->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

    // 3) SLERP Drive 설정
    //joint->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(50.0f, 5.0f, FLT_MAX,true) );

    // --- 바닥용 Plane Actor ---
    PxPlane plane(0, 0, 1, 0);
    PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, plane, *gMaterial);
    gScene->addActor(*groundPlane);
}

void FPhysScene::Simulate(float DeltaTime)
{
    gScene->simulate(DeltaTime * 2.5f);
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
