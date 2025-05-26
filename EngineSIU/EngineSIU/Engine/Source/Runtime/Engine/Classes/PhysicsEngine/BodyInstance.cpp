#include "BodyInstance.h"

FBodyInstance::FBodyInstance()
    : InstanceBodyIndex(INDEX_NONE)
    , InstanceBoneIndex(INDEX_NONE)
    , CollisionEnabled(ECollisionEnabled::QueryAndPhysics)
    , DOFMode(EDOFMode::Default)
    , bUseCCD(false)
    , bLockTranslation(true)
    , bLockRotation(true)
    , bLockXTranslation(false)
    , bLockYTranslation(false)
    , bLockZTranslation(false)
    , bLockXRotation(false)
    , bLockYRotation(false)
    , bLockZRotation(false)
    , bOverrideMaxAngularVelocity(false)
    , Scale3D(1.0f)
    , LinearDamping(0.01)
    , AngularDamping(0.0)
    , COMNudge(0.f)
    , MassScale(1.f)
    , DOFConstraint(nullptr)
    , OwnerComponent(nullptr)
    , SourceObject(nullptr)
{
}



