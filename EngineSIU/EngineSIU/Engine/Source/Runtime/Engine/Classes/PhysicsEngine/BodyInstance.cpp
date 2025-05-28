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

void FBodyInstance::InitBody(UBodySetup* InBodySetup, const FTransform& InBodyWorldTransform, FPhysScene* InScene)
{ //TODO: Position말고 Transform받아서 회전값도 적용
    if (!InBodySetup || !InScene)
    {
        UE_LOG(ELogLevel::Error, TEXT("FBodyInstance::InitBody : InBodySetup or InScene is nullptr"));
        return;
    }

    MyScene = InScene;
    
    BodySetup = InBodySetup;
    
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
    PxTransform pose = PxTransform(InBodyWorldTransform.GetLocation().ToPxVec3(), InBodyWorldTransform.GetRotation().ToPxQuat());
    RigidBody = InScene->gPhysics->createRigidDynamic(pose);
    AttachShapes(InBodySetup->AggGeom, InScene);
    RigidBody->setSolverIterationCounts(8, 2);
    RigidBody->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, false);
    RigidBody->setMaxDepenetrationVelocity(2.f);

    RigidBody->setAngularDamping(2.0f); // 강한 회전 감쇠
    RigidBody->setLinearDamping(1.0f);  // 선형 감쇠도 안정성 증가

    // @@ TODO : TEst 용 추가
    //RigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

    // Shape 생성
    RigidBody->setLinearVelocity(PxVec3(0, 0, 0));
    RigidBody->setAngularVelocity(PxVec3(0, 0, 0));

    float Volume = InBodySetup->AggGeom.TotalVolume;
    float Mass = FMath::Max(Volume * 10.f, 0.01f);
    PxRigidBodyExt::updateMassAndInertia(*RigidBody, Mass);
    //PxRigidBodyExt::updateMassAndInertia(*RigidBody, 10.0f);

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
        PxQuat AdjustedRotation = CapsuleRotation * PxQuat(PxPi / 2, PxVec3(0, 1, 0)); // Z축→Y축 보정
        AdjustedRotation = CapsuleRotation;
        // 3. Actor 자체를 이동시킴 (ShapePose가 아닌 ActorPose)
        //PxTransform ActorPose(CapsuleCenter, CapsuleRotation);
        //RigidBody = InScene->gPhysics->createRigidDynamic(ActorPose);
        //RigidBody->setGlobalPose(InBodyWorldTransform.ToPxTransform());

        // 4. Shape은 Actor 기준으로 위치 0으로 고정
        PxCapsuleGeometry Geometry(Radius, HalfLength);
        PxShape* Shape = InScene->gPhysics->createShape(Geometry, *InScene->gMaterial);
        //Shape->setLocalPose(PxTransform(CapsuleCenter,CapsuleRotation));
        Shape->setLocalPose(PxTransform(CapsuleCenter, AdjustedRotation));

        Shape->setContactOffset(0.05f);  // 충돌 감지 시작 거리
        Shape->setRestOffset(0.01f);     // solver에서 penetration 허용 오차

        RigidBody->attachShape(*Shape);
        Shape->release();
    }
}

UBodySetup* FBodyInstance::GetBodySetup() const
{
    if (UBodySetupCore* BodySetupCore = BodySetup)
    {
        return CastChecked<UBodySetup>(BodySetupCore);
    }
    return nullptr;
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
