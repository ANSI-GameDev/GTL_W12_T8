#pragma once
#include "PhysicsCore/BodySetupCore.h"
#include "AggregateGeom.h"

class UPhysicalMaterial;
/* 실제 물리 Body 생성 및 시뮬레이션 적용
 * 일반 물리 Body에서만 사용
 */

enum EPhysAssetFitGeomType : int
{
    EFG_Box				,
    EFG_Sphyl			,
    EFG_Sphere			,
    EFG_TaperedCapsule	,
    EFG_SingleConvexHull,
    EFG_MultiConvexHull	,
    EFG_LevelSet		,
    EFG_SkinnedLevelSet	,
};


class UBodySetup : public UBodySetupCore
{
    DECLARE_CLASS(UBodySetup, UBodySetupCore)
public:
    UBodySetup() = default;

    /* 위 PhysicAsset의 Bounding Box 계산 시 고려 여부 : false면 경계 박스 계산에서 제외*/
    uint8 bConsiderForBounds : 1;

    /* PhysX 트라이앵글 메시에서 양면 충돌 여부 설정*/
    uint8 bDoubleSidedGeometry : 1;

    /* PhysX 충돌 메시(Convex, TriangleMesh) 생성 완료 플래그.*/
    uint8 bCreatedPhysicsMeshes : 1;

    /* PhysX 충돌 메시 생성 실패 여부 확인용. */
    uint8 bFailedToCreatePhysicsMeshes : 1;


    /* PhysX 콜리전 메시 빌드시 스케일 적용.*/
    FVector BuildScale;

    FKAggregateGeom AggGeom;

    UPhysicalMaterial* PhysMaterial;

    /* Body 생성 시 사용할 기본 Geomtery 종류 : Param 으로 따로 분리 필요 */
    EPhysAssetFitGeomType GeomType = EPhysAssetFitGeomType::EFG_Sphyl; 
};
