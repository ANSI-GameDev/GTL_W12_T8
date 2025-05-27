#pragma once
#include "GameFramework/Actor.h"

class UCapsuleComponent;


class ACapsuleActor : public AActor
{
    DECLARE_CLASS(ACapsuleActor, AActor)

public:
    ACapsuleActor();

    void InitBodyInstance();
    
    UCapsuleComponent* GetShapeComponent() const;

protected:
    UPROPERTY(
        VisibleAnywhere,
        UCapsuleComponent*, CapsuleComponent, = nullptr;
    )
};

