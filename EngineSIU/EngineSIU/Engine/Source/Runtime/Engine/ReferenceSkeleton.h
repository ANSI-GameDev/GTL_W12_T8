#pragma once
#include "CoreMiscDefines.h"
#include "Container/Array.h"
#include "Container/Map.h"
#include "UObject/NameTypes.h"
#include "Math/Transform.h"

struct FMeshBoneInfo
{
    // Bone's name.
    FName Name;

    // INDEX_NONE if this is the root bone. 
    int32 ParentIndex;

    FMeshBoneInfo() : Name(NAME_None), ParentIndex(INDEX_NONE) {}

    FMeshBoneInfo(const FName& InName, int32 InParentIndex)
        : Name(InName)
        , ParentIndex(InParentIndex)
    {}

    bool operator==(const FMeshBoneInfo& B) const
    {
        return(Name == B.Name);
    }

    friend FArchive& operator<<(FArchive& Ar, FMeshBoneInfo& Info)
    {
        return Ar << Info.Name 
                  << Info.ParentIndex;
    }
};

struct FReferenceSkeleton
{
public:
    TArray<FMeshBoneInfo> RawRefBoneInfo;
    TArray<FTransform> RawRefBonePose;

    TArray<FMatrix> InverseBindPoseMatrices;

    TMap<FName, int32> RawNameToIndexMap;

    int32 FindBoneIndex(const FName& BoneName) const;

    int32 GetRawBoneNum() const
    {
        return RawRefBoneInfo.Num();
    }

    const TArray<FMeshBoneInfo> & GetRawRefBoneInfo() const
    {
        return RawRefBoneInfo;
    }

    const TArray<FTransform> & GetRawRefBonePose() const
    {
        return RawRefBonePose;
    }
    
    TArray<FName> GetRawRefBoneNames() const
    {
        TArray<FName> BoneNames;
        BoneNames.Reserve(RawRefBoneInfo.Num());

        for (const FMeshBoneInfo& BoneInfo : RawRefBoneInfo)
        {
            BoneNames.Add(BoneInfo.Name);
        }

        return BoneNames;
    }
	
    void Empty(int32 Size=0)
    {
        RawRefBoneInfo.Empty(Size);
        RawRefBonePose.Empty(Size);

        InverseBindPoseMatrices.Empty(Size);

        RawNameToIndexMap.Empty(Size);
    }

    int32 FindRawBoneIndex(const FName& BoneName) const
    {
        int32 BoneIndex = INDEX_NONE;
        if (BoneName != NAME_None)
        {
            const int32* IndexPtr = RawNameToIndexMap.Find(BoneName);
            if (IndexPtr)
            {
                BoneIndex = *IndexPtr;
            }
        }
        return BoneIndex;
    }

    FName GetBoneName(const int32 BoneIndex) const
    {
        return RawRefBoneInfo[BoneIndex].Name;
    }

    bool IsValidRawIndex(int32 Index) const
    {
        return (RawRefBoneInfo.IsValidIndex(Index));
    }

    int32 GetParentIndexInternal(const int32 BoneIndex, const TArray<FMeshBoneInfo>& BoneInfo) const
    {
        const int32 ParentIndex = BoneInfo[BoneIndex].ParentIndex;

        assert(
                ((BoneIndex == 0) && (ParentIndex == INDEX_NONE)) ||
                ((BoneIndex > 0) && BoneInfo.IsValidIndex(ParentIndex) && (ParentIndex < BoneIndex))
            );
        
        return ParentIndex;
    }
    
    int32 GetParentIndex(const int32 BoneIndex) const
    {
        return GetParentIndexInternal(BoneIndex, RawRefBoneInfo);
    }
    FTransform GetRefWorldTransform (int32 BoneIndex) const
    {
        FTransform Result = RawRefBonePose[BoneIndex];
        int32 ParentIdx = GetParentIndex(BoneIndex);
        while (ParentIdx != INDEX_NONE)
        {
            Result = RawRefBonePose[ParentIdx] * Result;
            ParentIdx = GetParentIndex(ParentIdx);
        }
        return Result;
    }

    FTransform GetRefWorldTransform(int32 BoneIndex) const
    {
        FTransform Result = RawRefBonePose[BoneIndex];
        int32 ParentIdx = GetParentIndex(BoneIndex);
        while (ParentIdx != INDEX_NONE)
        {
            Result = RawRefBonePose[ParentIdx] * Result;
            ParentIdx = GetParentIndex(ParentIdx);
        }
        return Result;
    }

    void Serialize(FArchive& Ar)
    {
        Ar << RawRefBoneInfo
           << RawRefBonePose
           << InverseBindPoseMatrices
           << RawNameToIndexMap;
    }
};
