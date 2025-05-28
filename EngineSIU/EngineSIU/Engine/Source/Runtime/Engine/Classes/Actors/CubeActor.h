#pragma once
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"

class UBoxComponent;


class ACubeActor : public AStaticMeshActor
{
    DECLARE_CLASS(ACubeActor, AStaticMeshActor)

public:
    ACubeActor();
    
    void InitBodyInstance() override;
    void SetKinematic(bool bIsKinematic);

    virtual void Tick(float DeltaTime) override;
};

