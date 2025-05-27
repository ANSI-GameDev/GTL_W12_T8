#include "BodyInstance.h"

#include <PxPhysics.h>
#include <PxRigidDynamic.h>
#include <PxScene.h>
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
    if (!InBodySetup || !InScene)
    {
        UE_LOG(ELogLevel::Error, TEXT("FBodyInstance::InitBody : InBodySetup or InScene is nullptr"));
        return;
    }

    //등록하는 행위
    InScene->BodyInstances.Add(this);

    // 기존 RigidBody가 있다면 제거 및 해제
    // TODO : Joint 정보 또한 제거 필요
    if (RigidBody)
    {
        if (RigidBody->getScene())
        {
            RigidBody->getScene()->removeActor(*RigidBody);
        }
        RigidBody->release();
    }
    
    // Body의 위치 = Body가 속한 Bone의 World Position
    PxTransform pose = PxTransform(InBodyWorldPosition.ToPxVec3());
    RigidBody = InScene->gPhysics->createRigidDynamic(pose);
    // @@ TODO : TEst 용 추가
    //RigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

    // Shape 생성
    AttachShapes(InBodySetup->AggGeom, InScene);
    
    PxRigidBodyExt::updateMassAndInertia(*RigidBody, 10.0f);
    InScene->gScene->addActor(*RigidBody);
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

    for (const FKSphylElem& CapsuleGeom : InAggregateGeom.SphylElems)
    {
        // 1. Shape 정보
        PxReal Radius = CapsuleGeom.Radius;
        PxReal HalfLength = CapsuleGeom.Length * 0.5f;

        // 2. 캡슐 회전/위치
        PxVec3 CapsuleCenter = CapsuleGeom.Center.ToPxVec3();
        PxQuat CapsuleRotation = CapsuleGeom.RQuat.ToPxQuat();

        // 3. Actor 자체를 이동시킴 (ShapePose가 아닌 ActorPose)
        PxTransform ActorPose(CapsuleCenter, CapsuleRotation);
        RigidBody->setGlobalPose(ActorPose);

        // 4. Shape은 Actor 기준으로 위치 0으로 고정
        PxCapsuleGeometry Geometry(Radius, HalfLength);
        PxShape* Shape = InScene->gPhysics->createShape(Geometry, *InScene->gMaterial);
        Shape->setLocalPose(PxTransform(PxIdentity));

        RigidBody->attachShape(*Shape);
        Shape->release();
    }
}

physx::PxRigidDynamic* FBodyInstance::GetPxRigidBoDynamic() const
{
    if (!RigidBody)
    {
        UE_LOG(ELogLevel::Error, TEXT("FBodyInstance::GetPxRigidBoDynamic : RigidBody is nullptr"));
        return nullptr;
    }

    return RigidBody;
}

void FBodyInstance::UpdatePhysics()
{
    PxTransform t = RigidBody->getGlobalPose();
    
    WorldTransform = FTransform(t);
}
