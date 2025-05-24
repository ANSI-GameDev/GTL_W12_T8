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

void FBodyInstance::InitBody(UBodySetup* InBodySetup, const FVector& InBodyVec, FPhysScene* InScene)
{
    //아래는 하면 안될수도 있음
    MyScene = InScene;
    //주어진 BodySetup만 초기화됨. 기존에 있던 데이터는 사라짐
    
    //Instance의 위치주기
    PxTransform pose;
    ConvertFVecToPxVec(pose.p, InBodyVec);
    RigidBody = InScene->gPhysics->createRigidDynamic(pose);

    FKAggregateGeom Geometries = InBodySetup->AggGeom;

    //일단 박스만
    for (FKBoxElem BoxGeom : Geometries.BoxElems)
    {
        FVector FVecHalfExt(BoxGeom.X, BoxGeom.Y, BoxGeom.Z);
        PxVec3 halfExtent;
        ConvertFVecToPxVec(halfExtent, FVecHalfExt);
        PxShape* shape = InScene->gPhysics->createShape(PxBoxGeometry(halfExtent), *InScene->gMaterial);
        RigidBody->attachShape(*shape);
        shape->release();
    }
    
    PxRigidBodyExt::updateMassAndInertia(*RigidBody, 10.0f);
    InScene->gScene->addActor(*RigidBody);
    UpdatePhysics();
}

void FBodyInstance::UpdatePhysics()
{
    // SCOPED_READ_LOCK(*MyScene->gScene)
    // if (RigidActor->is<PxRigidDynamic>())
    {
        PxTransform t = RigidBody->getGlobalPose();
        // PxMat44 mat(t);
        // ConvertPxMatToFMat(WorldMatrix, mat);
        
        ConvertPxTransformToFTransform(WorldTransform, t);
    }
}

void FBodyInstance::ConvertPxTransformToFTransform(FTransform& OutTransform, const PxTransform& InTransform)
{
    OutTransform.Translation = FVector(InTransform.p.x, InTransform.p.y, InTransform.p.z);
    OutTransform.Rotation = FQuat(InTransform.q.x, InTransform.q.y, InTransform.q.z, InTransform.q.w);
    OutTransform.Scale3D = FVector::OneVector;
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

void FBodyInstance::ConvertFVecToPxVec(PxVec3& OutVec, const FVector& InVec)
{
    OutVec = PxVec3(InVec.X, InVec.Y, InVec.Z);
}
