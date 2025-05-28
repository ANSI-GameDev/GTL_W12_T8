#pragma once

#include "UObject/NameTypes.h"

class USkeletalMesh;
class USkeletalMeshComponent;

namespace PhysicsSettingsSerializer
{
    void SavePhysicsSettings(USkeletalMeshComponent* SkeletalMeshComponent);
    bool LoadPhysicsSettings(USkeletalMesh* SkeletalMesh, const FString& MeshPath);
}
