#include "BodyInstance.h"

#include <PxPhysics.h>
#include <PxRigidDynamic.h>
#include <PxScene.h>
#include <extensions/PxRigidBodyExt.h>
#include <foundation/PxMat44.h>

#include "AggregateGeom.h"
#include "BodySetup.h"
#include "Components/SceneComponent.h"
#include "Math/JungleMath.h"
#include "PhysicsCore/Public/Physics/PhysScene.h"

using namespace physx;

#define SCOPED_READ_LOCK(scene) PxSceneReadLock scopedReadLock(scene);

void FBodyInstance::SetTransformRigidBody(FTransform MoveLocation)
{
    RigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    RigidBody->setKinematicTarget(ConvertFTransformToPxTransform(MoveLocation));
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
    PxTransform pose = PxTransform(ConvertFVecToPxVec(InBodyWorldPosition));
    RigidBody = InScene->gPhysics->createRigidDynamic(pose);

    AttachShapes(InBodySetup->AggGeom, InScene);
    //일단 박스만
    
    PxRigidBodyExt::updateMassAndInertia(*RigidBody, 10.0f);
    InScene->gScene->addActor(*RigidBody);
    UpdatePhysics();
}

void FBodyInstance::AttachShapes(const FKAggregateGeom& InAggregateGeom, FPhysScene* InScene)
{
    for (FKBoxElem BoxGeom : InAggregateGeom.BoxElems)
    {
        FVector FVecHalfExt = BoxGeom.Extent/2;
        PxVec3 halfExtent = ConvertFVecToPxVec(FVecHalfExt);
        PxTransform ShapePose = PxTransform(ConvertFVecToPxVec(BoxGeom.Center));
        PxShape* Shape = InScene->gPhysics->createShape(PxBoxGeometry(halfExtent), *InScene->gMaterial);
        Shape->setLocalPose(ShapePose);
        RigidBody->attachShape(*Shape);
        Shape->release();
    }

    for (FKSphereElem SphereGeom : InAggregateGeom.SphereElems)
    {
        PxReal Radius = SphereGeom.Radius;
        PxTransform ShapePose = PxTransform(ConvertFVecToPxVec(SphereGeom.Center));
        PxShape* Shape = InScene->gPhysics->createShape(PxSphereGeometry(Radius), *InScene->gMaterial);
        Shape->setLocalPose(ShapePose);
        RigidBody->attachShape(*Shape);
        Shape->release();
    }

    for (FKSphylElem CapsuleGeom : InAggregateGeom.CapsuleElems)
    {
        PxReal Radius = CapsuleGeom.Radius;
        PxReal HalfLength = CapsuleGeom.Length/2;
        PxTransform ShapePose = PxTransform(ConvertFVecToPxVec(CapsuleGeom.Center));
        PxShape* Shape = InScene->gPhysics->createShape(PxCapsuleGeometry(Radius, HalfLength), *InScene->gMaterial);
        Shape->setLocalPose(ShapePose);
        RigidBody->attachShape(*Shape);
        Shape->release();
    }
}

void FBodyInstance::UpdatePhysics()
{
    PxTransform t = RigidBody->getGlobalPose();
    
    WorldTransform = ConvertPxTransformToFTransform(t);
}

FTransform FBodyInstance::ConvertPxTransformToFTransform(const PxTransform& InTransform)
{
    FTransform OutTransform;
    OutTransform.Translation = FVector(InTransform.p.x, InTransform.p.y, InTransform.p.z);
    OutTransform.Rotation = FQuat(InTransform.q.x, InTransform.q.y, InTransform.q.z, InTransform.q.w);
    OutTransform.Scale3D = FVector::OneVector;
    return OutTransform;
}

PxTransform FBodyInstance::ConvertFTransformToPxTransform(const FTransform& InTransform)
{
    PxTransform OutTransform;
    OutTransform.p = PxVec3(InTransform.Translation.X, InTransform.Translation.Y, InTransform.Translation.Z);
    OutTransform.q = PxQuat(InTransform.Rotation.X, InTransform.Rotation.Y, InTransform.Rotation.Z, InTransform.Rotation.W);
    return OutTransform;
}

void FBodyInstance::ConvertPxMatToFMat(FMatrix& OutFMatrix, physx::PxMat44 InMat)
{
    for (int i=0;i<4;i++)
    {
        for (int j=0;j<4;j++)
        {
            //PxMat는 열우선이고, FMatrix는 행우선임
            //InMat(row, col) return (*this)[col][row]
            //위 함수를 보면 전치해서 반환하기 때문에 그대로 i,j를 삽입하면 됨
            OutFMatrix.M[i][j] = InMat(i, j);
        }
    }
}

PxVec3 FBodyInstance::ConvertFVecToPxVec(const FVector& InVec)
{
    return PxVec3(InVec.X, InVec.Y, InVec.Z);
}
