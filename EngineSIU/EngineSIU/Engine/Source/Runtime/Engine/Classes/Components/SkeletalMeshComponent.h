#pragma once
#include "SkinnedMeshComponent.h"
#include "Actors/Player.h"
#include "Engine/AssetManager.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "Template/SubclassOf.h"
#include "Animation/AnimNodeBase.h"

class FPhysScene;
struct FConstraintInstance;
struct FBodyInstance;
class UPhysicsAsset;
class UAnimSequence;
class USkeletalMesh;
struct FAnimNotifyEvent;
class UAnimSequenceBase;
class UAnimInstance;
class UAnimSingleNodeInstance;

enum class EAnimationMode : uint8
{
    AnimationBlueprint,
    AnimationSingleNode,
};

class USkeletalMeshComponent : public USkinnedMeshComponent
{
    DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

public:
    USkeletalMeshComponent();
    virtual ~USkeletalMeshComponent() override = default;

    virtual void InitializeComponent() override;

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void TickComponent(float DeltaTime) override;
    virtual void UpdatePosePhysics();
    virtual void TickPose(float DeltaTime) override;
    void TickAnimation(float DeltaTime);
    void TickAnimInstances(float DeltaTime);

    bool ShouldTickAnimation() const;

    bool InitializeAnimScriptInstance();

    void ClearAnimScriptInstance();

    USkeletalMesh* GetSkeletalMeshAsset() const { return SkeletalMeshAsset; }
    void SetSkeletalMeshAsset(USkeletalMesh* InSkeletalMeshAsset);

    FTransform GetSocketTransform(FName SocketName) const;

    void GetCurrentGlobalBoneMatrices(TArray<FMatrix>& OutBoneMatrices) const;

    void DEBUG_SetAnimationEnabled(bool bEnable);

    void PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping);
    void SetAnimation(UAnimationAsset* NewAnimToPlay);
    UAnimationAsset* GetAnimation() const;

#pragma region Animation Events
    void Play(bool bLooping);
    void Stop();

    void SetPlaying(bool bPlaying);
    bool IsPlaying() const;

    void SetReverse(bool bIsReverse);
    bool IsReverse() const;

    void SetPlayRate(float Rate);
    float GetPlayRate() const;

    void SetLooping(bool bIsLooping);
    bool IsLooping() const;

    int GetCurrentKey() const;
    void SetCurrentKey(int InKey);

    void SetElapsedTime(float InElapsedTime);
    float GetElapsedTime() const;

    int32 GetLoopStartFrame() const;
    void SetLoopStartFrame(int32 InLoopStartFrame);

    int32 GetLoopEndFrame() const;
    void SetLoopEndFrame(int32 InLoopEndFrame);
    
    bool bIsAnimationEnabled() const { return bPlayAnimation; }

#pragma endregion
    
    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const override;

    const FSkeletalMeshRenderData* GetCPURenderData() const;

    static void SetCPUSkinning(bool Flag);
    static bool GetCPUSkinning();

    UAnimInstance* GetAnimInstance() const { return AnimScriptInstance; }
    void SetAnimationMode(EAnimationMode InAnimationMode);
    EAnimationMode GetAnimationMode() const { return AnimationMode; }

    virtual void InitAnim();

    UPhysicsAsset* GetPhysicsAsset() const;
    int32 FindRootBodyIndex() const;

    void OnCreatePhysicsState();
    void InitArticulated(FPhysScene* PhysScene);

    /* BodyInstance 생성 함수 호출 및 ConstraintInstance 생성 */
    void InstantiatePhysicsAsset_Internal(const UPhysicsAsset& PhysAsset, const FVector& Scale3D, TArray<FBodyInstance*>& OutBodies, TArray<FConstraintInstance*>& OutConstraints, FPhysScene* PhysScene /*= nullptr*/, USkeletalMeshComponent* OwningComponent /*= nullptr*/, int32 UseRootBodyIndex /*= INDEX_NONE*/) const;
    /* BodyInstance 생성 함수 */ 
    void InstantiatePhysicsAssetBodies_Internal(const UPhysicsAsset& PhysAsset, TArray<FBodyInstance*>& OutBodies, TMap<FName, FBodyInstance*>* OutNameToBodyMap, FPhysScene* PhysScene /*= nullptr*/, USkeletalMeshComponent* OwningComponent /*= nullptr*/, int32 UseRootBodyIndex /*= INDEX_NONE*/)const;

    //private로 옮겨야 함
    FPoseContext BonePoseContext;
protected:
    bool NeedToSpawnAnimScriptInstance() const;

    EAnimationMode AnimationMode;
    
private:
    void CPUSkinning(bool bForceUpdate = false);

    //FPoseContext BonePoseContext;

    USkeletalMesh* SkeletalMeshAsset;
    std::unique_ptr<FSkeletalMeshRenderData> CPURenderData;

    static bool bIsCPUSkinning;
    bool bPlayAnimation;


public:
    struct
    {
        int32 BodyIndex;
        FTransform TransformToRoot;
    } RootBodyData;

    TArray<FTransform> RefBonePoseTransforms;
    TArray<struct FBodyInstance*> Bodies;
    TArray<struct FConstraintInstance*> Constraints;

    TSubclassOf<UAnimInstance> AnimClass;    
    UAnimInstance* AnimScriptInstance;

    void SetAnimClass(UClass* NewClass);
    void SetAnimInstanceClass(class UClass* NewClass);
    UClass* GetAnimClass();    
    UAnimSingleNodeInstance* GetSingleNodeInstance() const;

private:
    int32 SelectedBoneIndex = INDEX_NONE;
public:
    void SetSelectedBone(int32 BoneIndex) { SelectedBoneIndex = BoneIndex; }
    int32 GetSelectedBone() const { return SelectedBoneIndex; }
};
