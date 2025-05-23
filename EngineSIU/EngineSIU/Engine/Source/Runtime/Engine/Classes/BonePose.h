#pragma once
#include "BoneContainer.h"
#include "Container/Array.h"
#include "Math/Transform.h"


struct FBasePose
{
    FORCEINLINE void InitBones(int32 NumBones)
    {
        Bones.Empty(NumBones);
        Bones.AddUninitialized(NumBones);
    }

    FORCEINLINE int32 GetNumBones() const
    {
        return Bones.Num();
    }

    FORCEINLINE bool IsValidIndex(const int32& BoneIndex) const
    {
        return Bones.IsValidIndex(BoneIndex);
    }

    FORCEINLINE FTransform& operator[](const int32& BoneIndex)
    { 
        return Bones[BoneIndex];
    }

    FORCEINLINE const FTransform& operator[] (const int32& BoneIndex) const
    {
        return Bones[BoneIndex];
    }

    FORCEINLINE TArray<FTransform> GetMutableBones()
    {
        return TArray<FTransform>(Bones);
    }

    const TArray<FTransform>& GetBones() const { return Bones; }

    TArray<FTransform>&& MoveBones() { return std::move(Bones); }

protected:
    TArray<FTransform> Bones;
};

struct FBaseCompactPose : public FBasePose
{
    FBaseCompactPose()
        : BoneContainer(nullptr)
    {}

    const FBoneContainer& GetBoneContainer() const
    {
        return *BoneContainer;
    }

    FBoneContainer& GetBoneContainer()
    {
        return *const_cast<FBoneContainer*>(BoneContainer);
    }

    void SetBoneContainer(const FBoneContainer* InBoneContainer)
    {
        BoneContainer = InBoneContainer;
        this->InitBones(BoneContainer->GetBoneIndicesArray().Num());
    }

    void CopyAndAssignBoneContainer(FBoneContainer& NewBoneContainer)
    {
        NewBoneContainer = *BoneContainer;
        BoneContainer = &NewBoneContainer;
    }

    void InitFrom(const FBaseCompactPose& SrcPose)
    {
        SetBoneContainer(SrcPose.BoneContainer);
        this->Bones = SrcPose.Bones;
    }

    void InitFrom(FBaseCompactPose&& SrcPose)
    {
        SetBoneContainer(SrcPose.BoneContainer);
        this->Bones = std::move(SrcPose.Bones);
        SrcPose.BoneContainer = nullptr;
    }
    
	void CopyBonesFrom(const FBaseCompactPose& SrcPose)
	{
		if (this != &SrcPose)
		{
			this->Bones = SrcPose.GetBones();
			BoneContainer = &SrcPose.GetBoneContainer();
		}
	}
	
	void MoveBonesFrom(FBaseCompactPose& SrcPose)
	{
		if (this != &SrcPose)
		{
			this->Bones = SrcPose.MoveBones();
			BoneContainer = &SrcPose.GetBoneContainer();
			SrcPose.BoneContainer = nullptr;
		}
	}

	void CopyBonesTo(TArray<FTransform>& DestPoseBones)
	{
		DestPoseBones = this->Bones;
	}

	// Moves transform data out of the supplied InTransforms. InTransform will be left empty
	void MoveBonesFrom(TArray<FTransform>&& InTransforms)
	{
		this->Bones = std::move(InTransforms);
	}

	// Moves transform data out of this to the supplied InTransforms. Bones will be left empty
	void MoveBonesTo(TArray<FTransform>& InTransforms)
	{
		InTransforms = std::move(this->Bones);
	}

	void Empty()
	{
		BoneContainer = nullptr;
		this->Bones.Empty();
	}

	// Sets this pose to its ref pose
	void ResetToRefPose()
	{
		ResetToRefPose(GetBoneContainer());
	}

	// Sets this pose to the supplied BoneContainers ref pose
	void ResetToRefPose(const FBoneContainer& RequiredBones)
	{
		RequiredBones.FillWithCompactRefPose(this->Bones);
	
		this->BoneContainer = &RequiredBones;
        
		// Only do this if we have a mesh. otherwise we're not retargeting animations.
		if (RequiredBones.GetSkeletalMeshAsset())
		{
			TArray<FTransform> const & SkeletonRefPose = RequiredBones.GetSkeletonAsset()->GetReferencePose();
	
			for (int32 BoneIndex = 0; BoneIndex< Bones.Num(); BoneIndex++)
			{
			    const int32 SkeletonBoneIndex = GetBoneContainer().GetSkeletonIndex(BoneIndex);
	
			    // Pose bone index should always exist in Skeleton
			    this->Bones[BoneIndex] = SkeletonRefPose[SkeletonBoneIndex];
			}
		}
	}

	// Sets every bone transform to Identity
	void ResetToAdditiveIdentity()
	{
		for (FTransform& Bone : this->Bones)
		{
			Bone.SetIdentityZeroScale();
		}
	}

	// returns true if all bone rotations are normalized
	bool IsNormalized() const
	{
		for (const FTransform& Bone : this->Bones)
		{
			if (!Bone.IsRotationNormalized())
			{
				return false;
			}
		}

		return true;
	}

	// Returns true if any bone rotation contains NaN or Inf
	bool ContainsNaN() const
	{
		for (const FTransform& Bone : this->Bones)
		{
			if (Bone.ContainsNaN())
			{
				return true;
			}
		}

		return false;
	}


	// Normalizes all rotations in this pose
	void NormalizeRotations()
	{
		for (FTransform& Bone : this->Bones)
		{
			Bone.NormalizeRotation();
		}
	}

	bool IsValid() const
	{
		return (BoneContainer && BoneContainer->IsValid());
	}

	// Returns the bone index for the parent bone
	int32 GetParentBoneIndex(const int32& BoneIndex) const
	{
		return GetBoneContainer().GetParentBoneIndex(BoneIndex);
	}

	// Returns the ref pose for the supplied bone
	const FTransform& GetRefPose(const int32& BoneIndex) const
	{
		return GetBoneContainer().GetRefPoseTransform(BoneIndex);
	}
    FORCEINLINE void SetBoneTransform(const int32 BoneIndex, const FTransform& NewTransform)
    {
        if (IsValidIndex(BoneIndex))
        {
            Bones[BoneIndex] = NewTransform;
        }
    }

    FORCEINLINE FTransform& GetBoneTransform(const int32 BoneIndex)
    {
        return Bones[BoneIndex];
    }

    FORCEINLINE const FTransform& GetBoneTransform(const int32 BoneIndex) const
    {
        return Bones[BoneIndex];
    }

protected:
    
    const FBoneContainer* BoneContainer;
};

struct FCompactPose : public FBaseCompactPose
{
    // Sets every bone transform to Identity
    void ResetToAdditiveIdentity();

    // Normalizes all rotations in this pose
    void NormalizeRotations();
};
