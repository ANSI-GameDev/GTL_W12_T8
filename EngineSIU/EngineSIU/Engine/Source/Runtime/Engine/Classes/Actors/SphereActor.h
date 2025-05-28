#pragma once
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"

class USphereComponent;


class ASphereActor : public AStaticMeshActor
{
    DECLARE_CLASS(ASphereActor, AStaticMeshActor)
public:
    ASphereActor();
    void InitBodyInstance() override;
    virtual void Tick(float DeltaTime) override;
};

