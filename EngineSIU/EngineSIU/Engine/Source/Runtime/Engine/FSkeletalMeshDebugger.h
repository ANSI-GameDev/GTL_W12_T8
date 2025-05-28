#pragma once

#include "Define.h"
#include "Components/SkeletalMeshComponent.h"

class FSkeletalMeshDebugger
{
public:
    static void DrawSkeleton(const USkeletalMeshComponent* SkelMeshComp, UPrimitiveDrawBatch* DrawBatch);
    static void DrawSkeletonAABBs(const USkeletalMeshComponent* SkelMeshComp, UPrimitiveDrawBatch* DrawBatch);
    static void DrawConeConstraints(
        const USkeletalMeshComponent* SkelComp,
        UPrimitiveDrawBatch* DrawBatch,
        const FName& SelectedConstraintName,
        bool bDrawAll);
    static void DrawBodyShapes(
        const USkeletalMeshComponent* SkelComp,
        UPrimitiveDrawBatch* DrawBatch,
        const FName& SelectedBodyName,
        bool bDrawAll);
}; 
