#include "BodyInstance.h"

#include <PxPhysics.h>
#include <PxRigidDynamic.h>
#include <PxScene.h>
#include <PxSceneLock.h>
#include <extensions/PxRigidBodyExt.h>

#include "AggregateGeom.h"
#include "BodySetup.h"
#include "Physics/PhysScene.h"
#include "Developer/PhysicsUtilities/PxConvertHelper.inl"

using namespace physx;

#define SCOPED_READ_LOCK(scene) PxSceneReadLock scopedReadLock(scene);

void FBodyInstance::SetTransformRigidBody(FTransform MoveLocation)
{
    RigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    RigidBody->setKinematicTarget(MoveLocation.ToPxTransform());
    RigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
}

void FBodyInstance::InitBody(UBodySetup* InBodySetup, const FVector& InBodyWorldPosition, FPhysScene* InScene)
{
    //아래는 하면 안될수도 있음
    // MyScene = InScene;

    //등록하는 행위
    InScene->BodyInstances.Add(this);

    //주어진 BodySetup만 초기화됨. 기존에 있던 데이터는 사라짐
    //있는데 다시 만들면 해제했다가 다시 할당
    if (RigidBody)
    {
        if (RigidBody->getScene())
        {
            RigidBody->getScene()->removeActor(*RigidBody);
        }
        //joint가 있으면 joint도 같이 해제해야함
        RigidBody->release();
    }
    
    //Instance의 위치주기
    PxTransform pose = PxTransform(InBodyWorldPosition.ToPxVec3());
    RigidBody = InScene->gPhysics->createRigidDynamic(pose);

    AttachShapes(InBodySetup->AggGeom, InScene);
    //일단 박스만
    
    PxRigidBodyExt::updateMassAndInertia(*RigidBody, 10.0f);
    InScene->gScene->addActor(*RigidBody);
    UpdatePhysics();
}

void FBodyInstance::InitBody(PxRigidDynamic* InBody, const FVector& InBodyWorldPosition, FPhysScene* InScene)
{
    InScene->BodyInstances.Add(this);
    if (RigidBody)
    {
        if (RigidBody->getScene())
        {
            RigidBody->getScene()->removeActor(*RigidBody);
        }
        RigidBody->release();
    }
    RigidBody = InBody;
    
    {
        PxSceneWriteLock scopedLock(*InScene->gScene);
        InScene->gScene->addActor(*RigidBody);
        RigidBody->setGlobalPose(PxTransform(InBodyWorldPosition.ToPxVec3()));
    }
    UpdatePhysics();
}

void FBodyInstance::AttachShapes(const FKAggregateGeom& InAggregateGeom, FPhysScene* InScene)
{
    for (FKBoxElem BoxGeom : InAggregateGeom.BoxElems)
    {
        FVector FVecHalfExt = BoxGeom.Extent/2;
        PxVec3 halfExtent = FVecHalfExt.ToPxVec3();
        PxTransform ShapePose = PxTransform(BoxGeom.Center.ToPxVec3());
        PxShape* Shape = InScene->gPhysics->createShape(PxBoxGeometry(halfExtent), *InScene->gMaterial);
        Shape->setLocalPose(ShapePose);
        RigidBody->attachShape(*Shape);
        Shape->release();
    }

    for (FKSphereElem SphereGeom : InAggregateGeom.SphereElems)
    {
        PxReal Radius = SphereGeom.Radius;
        PxTransform ShapePose = PxTransform(SphereGeom.Center.ToPxVec3());
        PxShape* Shape = InScene->gPhysics->createShape(PxSphereGeometry(Radius), *InScene->gMaterial);
        Shape->setLocalPose(ShapePose);
        RigidBody->attachShape(*Shape);
        Shape->release();
    }

    for (FKSphylElem CapsuleGeom : InAggregateGeom.SphylElems)
    {
        PxReal Radius = CapsuleGeom.Radius;
        PxReal HalfLength = CapsuleGeom.Length/2;
        PxTransform ShapePose = PxTransform(CapsuleGeom.Center.ToPxVec3());
        PxShape* Shape = InScene->gPhysics->createShape(PxCapsuleGeometry(Radius, HalfLength), *InScene->gMaterial);
        Shape->setLocalPose(ShapePose);
        RigidBody->attachShape(*Shape);
        Shape->release();
    }
}

void FBodyInstance::UpdatePhysics()
{
    PxTransform t = RigidBody->getGlobalPose();
    
    WorldTransform = FTransform(t);
}
