#pragma once
#include "Define.h"
#include "ShapeElem.h"
#include "Container/Array.h"
#include "Math/Transform.h"

struct FKConvexElem : public FKShapeElem
{
    FKConvexElem();
    virtual ~FKConvexElem() override;

    void  UpdateElemBox();
    FBoundingBox CalcAABB(const FTransform& BoneTM, const FVector& Scale3D) const;
    int32 GetNumVertices() const { return VertexData.Num(); }

public:
    TArray<FVector> VertexData;
    TArray<int32> IndexData;
    FBoundingBox ElemBox;

private:
    FTransform Transform;
};
