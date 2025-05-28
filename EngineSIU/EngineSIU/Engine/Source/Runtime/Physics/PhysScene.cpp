#include "PhysScene.h"

#include "UObject/Casts.h"
#include "World/World.h"

#include <PxPhysicsAPI.h>

using namespace physx;
PxFoundation* FPhysXGlobals::Foundation = nullptr;
PxPvd* FPhysXGlobals::Pvd = nullptr;

PxFoundation* FPhysXGlobals::GetFoundation()
{
    if (!Foundation)
    {
        static PxDefaultAllocator GAllocator;
        static PxDefaultErrorCallback GErrorCallback;

        Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, GAllocator, GErrorCallback);
    }
    return Foundation;
}

PxPvd* FPhysXGlobals::GetPvd()
{
    if (!Pvd)
    {
        PxFoundation* Foundation = GetFoundation();
        Pvd = PxCreatePvd(*Foundation);
        PxPvdTransport* Transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
        Pvd->connect(*Transport, PxPvdInstrumentationFlag::eALL);
    }
    return Pvd;
}
void FPhysScene::TickPhysScene(float DeltaTime)
{
    Simulate(DeltaTime);
}

void FPhysScene::WaitPhysScenes()
{
}

void FPhysScene::InitPhysX()
{
    using namespace physx;

    PxFoundation* Foundation = FPhysXGlobals::GetFoundation();
    PxPvd* Pvd = FPhysXGlobals::GetPvd();

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), true, Pvd);
    if (!gPhysics)
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create PxPhysics!"));
        return;
    }

    PxInitExtensions(*gPhysics, Pvd);

    gDispatcher = PxDefaultCpuDispatcherCreate(2);

    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    PxSceneDesc SceneDesc(gPhysics->getTolerancesScale());
    SceneDesc.gravity = PxVec3(0.0f, 0.0f, -9.81f);
    SceneDesc.cpuDispatcher = gDispatcher;
    SceneDesc.filterShader = PxDefaultSimulationFilterShader;
    SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS | PxSceneFlag::eENABLE_CCD | PxSceneFlag::eENABLE_PCM;

    gScene = gPhysics->createScene(SceneDesc);

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
       
    // PxRigidStatic* rigidStatic = nullptr;
    // PxPlane plane = PxPlane(0, 0, 1, -10);
    // rigidStatic = PxCreatePlane(*gPhysics, plane, *gMaterial);
    // gScene->addActor(*rigidStatic);
    
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
}


void FPhysScene::SetOwningWorld(UWorld* InOwningWorld)
{
    Owner = InOwningWorld;
}

UWorld* FPhysScene::GetOwningWorld() const
{
    return Cast<UWorld>(Owner);
}
