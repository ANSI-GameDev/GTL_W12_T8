#include "BodySetup.h"
#include "ConvexElem.h"
#include "SphereElem.h"
#include "BoxElem.h"

FKShapeElem::~FKShapeElem()
{
}

FKSphereElem::~FKSphereElem()
{
}

FKConvexElem::FKConvexElem()
    : FKShapeElem(EAggCollisionShape::Convex)
    , Transform(FTransform::Identity)
{
    
}

FKConvexElem::~FKConvexElem()
{
    VertexData.Empty();
    IndexData.Empty();
}

void FKConvexElem::UpdateElemBox()
{
    ElemBox.Init();
    for (int32 j = 0; j < VertexData.Num(); ++j)
    {
        ElemBox += VertexData[j];
    }
}

// TODO : ConvexElem의 Transform을 적용한 AABB 계산 -  (아래 return 수정 필요)
FBoundingBox FKConvexElem::CalcAABB(const FTransform& BoneTM, const FVector& Scale3D) const
{
    const FTransform LocalToWorld = FTransform(FQuat::Identity, FVector::ZeroVector, Scale3D) * BoneTM;

    return FBoundingBox(
        LocalToWorld.TransformPosition(ElemBox.MinLocation),
        LocalToWorld.TransformPosition(ElemBox.MaxLocation)
    );
}
