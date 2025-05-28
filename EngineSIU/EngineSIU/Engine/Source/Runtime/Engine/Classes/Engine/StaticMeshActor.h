#pragma once
#include "GameFramework/Actor.h"

class UStaticMeshComponent;


class AStaticMeshActor : public AActor
{
    DECLARE_CLASS(AStaticMeshActor, AActor)

public:
    AStaticMeshActor();
    virtual void InitBodyInstance(){}
    UStaticMeshComponent* GetStaticMeshComponent() const;

protected:
    UPROPERTY(
        VisibleAnywhere,
        UStaticMeshComponent*, StaticMeshComponent, = nullptr;
    )
};
