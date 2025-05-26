#pragma once
#include "Math/Quat.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "ReferenceSkeleton.h"

class USkeleton : public UObject
{
    DECLARE_CLASS(USkeleton, UObject)

public:
    USkeleton() = default;
    virtual ~USkeleton() override = default;

    FReferenceSkeleton GetRefSkeleton() const;

    const FReferenceSkeleton& GetRefSkeleton();

    const TArray<FTransform>& GetReferencePose() const;

    void SetReferenceSkeleton(const FReferenceSkeleton& InReferenceSkeleton)
    {
        ReferenceSkeleton = InReferenceSkeleton;
    }

    int32 FindBoneIndex(const FName& BoneName) const;

    virtual void SerializeAsset(FArchive& Ar) override;

protected:
    FReferenceSkeleton ReferenceSkeleton;
    
};
