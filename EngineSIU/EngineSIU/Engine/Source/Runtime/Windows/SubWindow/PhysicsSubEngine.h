// PhysicsSubEngine.h
#pragma once
#include "SubEngine.h"

class UPhysicsSubEngine : public USubEngine
{
    DECLARE_CLASS(UPhysicsSubEngine, USubEngine)
public:
    UPhysicsSubEngine();
    ~UPhysicsSubEngine();

    virtual void Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow, UnrealEd* InUnrealEd) override;
    virtual void Tick(float DeltaTime) override;
    virtual void Input(float DeltaTime) override;
    virtual void Render() override;
    virtual void Release() override;

public:
    USkeletalMeshComponent* GetSkeletalMeshComponent() const;
private:
    USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
private:
    const float CameraSpeedMultiplier = 10;
    // 예: PhysicsWorld, CollisionDebugRenderer 등 필요시 추가
};
