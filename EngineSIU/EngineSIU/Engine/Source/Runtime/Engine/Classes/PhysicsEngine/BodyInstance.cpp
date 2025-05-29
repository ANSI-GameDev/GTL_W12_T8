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

void FBodyInstance::SetTransformRigidBody(FTransform NewTransform)
{
    if (WorldTransform == NewTransform)
    {
        return;
    }

    if (RigidBody->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
    {
        RigidBody->setKinematicTarget(NewTransform.ToPxTransform());
        WorldTransform = NewTransform;
        return;
    }
    
    LinearVelocity = (NewTransform.Translation - WorldTransform.Translation);
    RigidBody->setLinearVelocity(LinearVelocity.ToPxVec3());
    
    FQuat DeltaQuat = NewTransform.Rotation * WorldTransform.Rotation.Inverse();

    FVector Axis;
    float Angle;
    DeltaQuat.ToAxisAndAngle(Axis, Angle);

    float DeltaTime = 1.f / 60.f;
    AngularVelocity = Axis * (Angle / DeltaTime);
    
    RigidBody->setAngularVelocity(AngularVelocity.ToPxVec3());
}

void FBodyInstance::SetRigidbodyKinematic(bool bIsKinematic)
{
    RigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, bIsKinematic);
    BodySetup->PhysicsType = bIsKinematic ? PhysType_Kinematic : PhysType_Default;
}

void FBodyInstance::InitBody(UBodySetup* InBodySetup, const FTransform& InBodyWorldTransform, FPhysScene* InScene, FTransform DefaultBodyTransform)
{ //TODO: Position말고 Transform받아서 회전값도 적용
    if (!InBodySetup || !InScene)
    {
        UE_LOG(ELogLevel::Error, TEXT("FBodyInstance::InitBody : InBodySetup or InScene is nullptr"));
        return;
    }

    MyScene = InScene;
    
    BodySetup = InBodySetup;

    OriginTransform = DefaultBodyTransform;
    
    //등록하는 행위
    InScene->BodyInstances.Add(this);

    // 기존 RigidBody가 있다면 제거 및 해제
    // TODO : Joint 정보 또한 제거 필요
    if (RigidBody)
    {
        DestroyInPhysicsScene();
    }
    
    // Body의 위치 = Body가 속한 Bone의 World Position
    PxTransform pose = PxTransform(InBodyWorldTransform.GetLocation().ToPxVec3(), InBodyWorldTransform.GetRotation().ToPxQuat());
    RigidBody = InScene->gPhysics->createRigidDynamic(pose);
    AttachShapes(InBodySetup->AggGeom, InScene);
    RigidBody->setSolverIterationCounts(16, 8);
    RigidBody->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, false);
    RigidBody->setMaxDepenetrationVelocity(1.f);

    RigidBody->setAngularDamping(1.0f); // 강한 회전 감쇠
    RigidBody->setLinearDamping(2.0f);  // 선형 감쇠도 안정성 증가
 
    // @@ TODO : TEst 용 추가
    //RigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    RigidBody->setMassSpaceInertiaTensor(PxVec3(0.01f, 0.01f, 0.01f));
    // Shape 생성
    RigidBody->setLinearVelocity(PxVec3(0, 0, 0));
    RigidBody->setAngularVelocity(PxVec3(0, 0, 0));

    float Volume = InBodySetup->AggGeom.TotalVolume;
    float Mass = FMath::Max(Volume * 10.f, 0.01f);  
    //PxRigidBodyExt::updateMassAndInertia(*RigidBody, Mass);
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
	// 코드 상단 혹은 전역에 collision 그룹 정의
	constexpr PxU32 CAPSULE_COLLISION_GROUP = (1 << 1);
	constexpr PxU32 ALL_COLLISION_GROUPS = 0xFFFFFFFF;

	// …in your loop over FKSphylElem…
	for (const FKSphylElem& CapsuleGeom : InAggregateGeom.SphylElems)
	{
		// 1) Capsule 생성
		PxReal Radius = CapsuleGeom.Radius;
		PxReal HalfLength = CapsuleGeom.Length * 0.5f;
		PxCapsuleGeometry Geometry(Radius, HalfLength);

		PxVec3  Center = CapsuleGeom.Center.ToPxVec3();
		PxQuat  LocalRotation = CapsuleGeom.RQuat.ToPxQuat();
		PxTransform ShapePose(Center, LocalRotation);

		// 2) Shape 생성
		PxShape* Shape = InScene->gPhysics->createShape(Geometry, *InScene->gMaterial);
		Shape->setLocalPose(ShapePose);
        Shape->setContactOffset(0.2f);
        Shape->setRestOffset(0.05f);

		// 3) 필터 설정: 같은 그룹(CAPSULE_COLLISION_GROUP)에 속한 것들끼리는 충돌하지 않도록
		PxFilterData fd;
		fd.word0 = CAPSULE_COLLISION_GROUP;             // this shape's own group
		fd.word1 = ALL_COLLISION_GROUPS ^ CAPSULE_COLLISION_GROUP;
		//      → collide with every group except CAPSULE_COLLISION_GROUP
		fd.word2 = 0;
		fd.word3 = 0;
		Shape->setSimulationFilterData(fd);
		Shape->setQueryFilterData(fd);  // 레이캐스트/오버랩에서도 동일 그룹 무시

		// 4) Actor에 부착
		RigidBody->attachShape(*Shape);
		Shape->release();
	}
    //for (const FKSphylElem& CapsuleGeom : InAggregateGeom.SphylElems)
    //{
    //    // 1. Shape 정보
    //    PxReal Radius = CapsuleGeom.Radius;
    //    PxReal HalfLength = CapsuleGeom.Length * 0.5f;

    //    // 2. 캡슐 회전/위치
    //    PxVec3 CapsuleCenter = CapsuleGeom.Center.ToPxVec3();
    //    PxQuat CapsuleRotation = CapsuleGeom.RQuat.ToPxQuat();
    //    PxQuat AdjustedRotation = CapsuleRotation * PxQuat(PxPi / 2, PxVec3(0, 1, 0)); // Z축→Y축 보정
    //    AdjustedRotation = CapsuleRotation;

    //    // 4. Shape은 Actor 기준으로 위치 0으로 고정
    //    PxCapsuleGeometry Geometry(Radius, HalfLength);
    //    PxShape* Shape = InScene->gPhysics->createShape(Geometry, *InScene->gMaterial);
    //    //Shape->setLocalPose(PxTransform(CapsuleCenter,CapsuleRotation));
    //    Shape->setLocalPose(PxTransform(CapsuleCenter, AdjustedRotation));

    //    Shape->setContactOffset(0.05f);  // 충돌 감지 시작 거리
    //    Shape->setRestOffset(0.01f);     // solver에서 penetration 허용 오차

    //    RigidBody->attachShape(*Shape);
    //    Shape->release();
    //    // filter
    //}
}

UBodySetup* FBodyInstance::GetBodySetup() const
{
    if (UBodySetupCore* BodySetupCore = BodySetup)
    {
        return CastChecked<UBodySetup>(BodySetupCore);
    }
    return nullptr;
}

PxRigidDynamic* FBodyInstance::GetPxRigidBoDynamic() const
{
    if (!RigidBody)
    {
        UE_LOG(ELogLevel::Error, TEXT("FBodyInstance::GetPxRigidBoDynamic : RigidBody is nullptr"));
        return nullptr;
    }

    return RigidBody;
}


void FBodyInstance::DestroyInPhysicsScene()
{
    if (!RigidBody)
    {
        return;
    }
    
    if (RigidBody->getScene())
    {
        RigidBody->getScene()->removeActor(*RigidBody);
    }

    MyScene->BodyInstances.Remove(this);
    RigidBody->release();
    RigidBody = nullptr;
}

void FBodyInstance::UpdatePhysics()
{
    if (!RigidBody)
    {
        return;
    }

    //Scale을 그대로 넘기기 위함
    FTransform RigidBodyWorldTransform = RigidBody->getGlobalPose();
    RigidBodyWorldTransform.Scale3D = OriginTransform.Scale3D;
    
    WorldTransform = RigidBodyWorldTransform;
    LinearVelocity = FVector(RigidBody->getLinearVelocity());
    AngularVelocity = FVector(RigidBody->getAngularVelocity());
}
