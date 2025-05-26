#pragma once

#include "Classes/Engine/StaticMeshActor.h"

class ACube : public AStaticMeshActor
{
    DECLARE_CLASS(ACube, AStaticMeshActor)

public:
    ACube();
    
    void InitCube();

    virtual void Tick(float DeltaTime) override;
};

