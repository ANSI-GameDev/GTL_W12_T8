#include "AnimDataController.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Misc/FrameTime.h"
#include "Math/MathUtility.h"

void UAnimDataController::SetModel(UAnimDataModel* InModel)
{
    Model = InModel;
}

void UAnimDataController::SetNumberOfFrames(int32 Length)
{
    const int32 CurrentNumberOfFrames = Model->GetNumberOfFrames();

    const int32 DeltaFrames = FMath::Abs(Length - CurrentNumberOfFrames);

    const int32 T0 = Length > CurrentNumberOfFrames ? CurrentNumberOfFrames : CurrentNumberOfFrames - DeltaFrames;
    const int32 T1 = Length > CurrentNumberOfFrames ? Length : CurrentNumberOfFrames;

    ResizeNumberOfFrames(Length, T0, T1);
}

void UAnimDataController::ResizeNumberOfFrames(int32 NewLength, int32 T0, int32 T1)
{
    if (NewLength >= 0)
    {
        if (NewLength != Model->GetNumberOfFrames())
        {
            if (T0 < T1)
            {
                Model->NumberOfFrames = NewLength;
                Model->NumberOfKeys = Model->NumberOfFrames + 1;
            }
        }
    }
}

void UAnimDataController::SetPlayLength(float Length)
{
    SetNumberOfFrames(ConvertSecondsToFrameNumber(Length));
}

int32 UAnimDataController::AddBoneTrack(FName BoneName)
{
    if (!Model->GetAnimationSequence())
    {
        return INDEX_NONE;
    }

    const int32 TrackIndex = Model->GetBoneTrackIndexByName(BoneName);

    if (TrackIndex == INDEX_NONE)
    {
        if (Model->GetNumBoneTracks() >= MAX_ANIMATION_TRACKS)
        {
            return INDEX_NONE;
        }
        else
        {
            const int32 InsertIndex = Model->BoneAnimationTracks.Num();
            int32 BoneIndex = INDEX_NONE;

            if (const UAnimSequence* AnimationSequence = Model->GetAnimationSequence())
            {
                if (const USkeleton* Skeleton = AnimationSequence->GetSkeleton())
                {
                    BoneIndex = Skeleton->GetRefSkeleton().FindBoneIndex(BoneName);
                }
            }

            if (BoneIndex != INDEX_NONE)
            {
                FBoneAnimationTrack NewTrack;
                NewTrack.Name = BoneName;
                NewTrack.BoneTreeIndex = BoneIndex;

                Model->BoneAnimationTracks.Emplace(NewTrack);

                return InsertIndex;
            }
        }
    }

    return TrackIndex;
}

bool UAnimDataController::RemoveBoneTrack(FName BoneName)
{
    if (!Model->GetAnimationSequence())
    {
        return false;
    }

    const FBoneAnimationTrack* ExistingTrackPtr = Model->FindBoneTrackByName(BoneName);

    if (ExistingTrackPtr != nullptr)
    {
        const int32 TrackIndex = Model->BoneAnimationTracks.IndexOfByPredicate([ExistingTrackPtr](const FBoneAnimationTrack& Track)
        {
            return Track.Name == ExistingTrackPtr->Name;
        });

        if (TrackIndex != INDEX_NONE)
        {
            return false;
        }

        TArray<FTransform> BoneTransforms;
        Model->GetBoneTrackTransforms(BoneName, BoneTransforms);

        Model->BoneAnimationTracks.RemoveAt(TrackIndex);

        return true;
    }

    return false;
}

void UAnimDataController::RemoveAllBoneTracks()
{
    if (!Model->GetAnimationSequence())
    {
        return;
    }
	
    TArray<FName> TrackNames;
    Model->GetBoneTrackNames(TrackNames);

    if (TrackNames.Num())
    {
        for (const FName& TrackName : TrackNames)
        {
            RemoveBoneTrack(TrackName);
        }
    }	
}

bool UAnimDataController::SetBoneTrackKeys(FName BoneName, const TArray<FVector>& PositionalKeys, const TArray<FQuat>& RotationalKeys, const TArray<FVector>& ScalingKeys)
{
    if (!Model->GetAnimationSequence())
    {
        return false;
    }

    // Validate key format
    const int32 MaxNumKeys = FMath::Max(FMath::Max(PositionalKeys.Num(), RotationalKeys.Num()), ScalingKeys.Num());

    if (MaxNumKeys > 0)
    {
        const bool bValidPosKeys = PositionalKeys.Num() == MaxNumKeys;
        const bool bValidRotKeys = RotationalKeys.Num() == MaxNumKeys;
        const bool bValidScaleKeys = ScalingKeys.Num() == MaxNumKeys;

        if (bValidPosKeys && bValidRotKeys && bValidScaleKeys)
        {
            if (FBoneAnimationTrack* TrackPtr = Model->FindMutableBoneTrackByName(BoneName))
            {
                TrackPtr->InternalTrackData.PosKeys.SetNum(MaxNumKeys);
                TrackPtr->InternalTrackData.ScaleKeys.SetNum(MaxNumKeys);
                TrackPtr->InternalTrackData.RotKeys.SetNum(MaxNumKeys);
                for(int32 KeyIndex = 0; KeyIndex<MaxNumKeys; KeyIndex++)
                {
                    TrackPtr->InternalTrackData.PosKeys[KeyIndex] = FVector(PositionalKeys[KeyIndex]);
                    TrackPtr->InternalTrackData.ScaleKeys[KeyIndex] = FVector(ScalingKeys[KeyIndex]);
                    TrackPtr->InternalTrackData.RotKeys[KeyIndex] = FQuat(RotationalKeys[KeyIndex]);
                }

                return true;
            }
        }
    }

    return false;
}

bool UAnimDataController::RemoveBoneTracksMissingFromSkeleton(const USkeleton* Skeleton)
{
    if (!Model->GetAnimationSequence())
    {
        return false;
    }

    if (Skeleton)
    {
        TArray<FName> TracksToBeRemoved;
        TArray<FName> TracksUpdated;
        const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetRefSkeleton();

        for (FBoneAnimationTrack& Track : Model->BoneAnimationTracks)
        {
            // Try find correct bone index
            const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(Track.Name);

            if (BoneIndex != INDEX_NONE && BoneIndex != Track.BoneTreeIndex)
            {
                // Update bone index
                Track.BoneTreeIndex = BoneIndex;
                TracksUpdated.Add(Track.Name);
            }
            else if (BoneIndex == INDEX_NONE)
            {				
                // Remove track
                TracksToBeRemoved.Add(Track.Name);
            }			
        }

        if (TracksToBeRemoved.Num() || TracksUpdated.Num())
        {
            for (const FName& TrackName : TracksToBeRemoved)
            {
                RemoveBoneTrack(TrackName);
            }
        }

        return TracksToBeRemoved.Num() > 0 || TracksUpdated.Num() > 0;
    }

    return false;
}

void UAnimDataController::UpdateWithSkeleton(USkeleton* TargetSkeleton)
{
    RemoveBoneTracksMissingFromSkeleton(TargetSkeleton);
}

inline FFrameTime AsFrameTime(int32 FrameRate, double TimeInSeconds)
{
    // TODO: 계산 식 수정해야 함.
    const int32 Numerator = 60000;
    const int32 Denominator = 1;
    
    const double TimeAsFrame = (TimeInSeconds * Numerator) / Denominator;
    int32 FrameNumber = static_cast<int32>(FMath::Clamp(FMath::FloorToDouble(TimeAsFrame), static_cast<double>(TNumericLimits<int32>::Min()), static_cast<double>(TNumericLimits<int32>::Max())));

    float SubFrame = static_cast<float>(TimeAsFrame - FMath::FloorToDouble(TimeAsFrame));
    const int32 TruncatedSubFrame = FMath::TruncToInt(SubFrame);
    SubFrame -= static_cast<float>(TruncatedSubFrame);
    FrameNumber += TruncatedSubFrame;
    if (SubFrame > 0.f)
    {
        SubFrame = FMath::Min(SubFrame, FFrameTime::MaxSubframe);
    }

    return FFrameTime(FrameNumber, SubFrame);
}

int32 UAnimDataController::ConvertSecondsToFrameNumber(double Seconds) const
{
    const int32& ModelFrameRate = GetModel()->GetFrameRate();
    
    const FFrameTime FrameTime = AsFrameTime(ModelFrameRate, Seconds);

    return FrameTime.GetFrame();
}

void UAnimDataController::SetFrameRate(int32 FrameRate)
{
    Model->FrameRate = FrameRate;
}
