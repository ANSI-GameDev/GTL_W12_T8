// PhysicsSettingsSerializer.cpp
#include "PhysicsSettingsSerializer.h"

#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/BodySetup.h"
#include "Components/SkeletalMeshComponent.h"
#include <fstream>

#include "Engine/SkeletalMesh.h"
#include "JSON/json.hpp"

using json = nlohmann::json;

namespace
{
    FString GetPhysicsSettingsPath(const FString& MeshName)
    {
        // 예: "Contents/Asset/Human" → "Contents/Asset/Human_PhysicsSettings.json"
        return MeshName + TEXT("_PhysicsSettings.json");
    }

}

namespace PhysicsSettingsSerializer
{
    void SavePhysicsSettings(USkeletalMeshComponent* SkelComp)
    {
        if (!SkelComp || !SkelComp->GetSkeletalMeshAsset()) return;
        UPhysicsAsset* PhysAsset = SkelComp->GetPhysicsAsset();
        if (!PhysAsset) return;
        //FString MeshName = SkelComp->GetSkeletalMeshAsset()->GetName();
        FString MeshName = SkelComp->GetSkeletalMeshAsset()->GetRenderData()->ObjectName;
        FString FilePath = GetPhysicsSettingsPath(MeshName);

        json Root;

        for (int32 i = 0; i < PhysAsset->BodySetup.Num(); ++i)
        {
            const UBodySetup* Body = PhysAsset->BodySetup[i];
            if (!Body) continue;

            json BodyJson;
            BodyJson["BoneName"] = (*Body->BoneName.ToString());

            for (const FKSphylElem& Sphyl : Body->AggGeom.SphylElems)
            {
                json Capsule;
                Capsule["Center"] = { Sphyl.Center.X, Sphyl.Center.Y, Sphyl.Center.Z };
                Capsule["Rotation"] = { Sphyl.RQuat.X, Sphyl.RQuat.Y, Sphyl.RQuat.Z, Sphyl.RQuat.W };
                Capsule["Radius"] = Sphyl.Radius;
                Capsule["Length"] = Sphyl.Length;
                BodyJson["Capsules"].push_back(Capsule);
            }
            //Sphere 저장
            for (const FKSphereElem& Sphere : Body->AggGeom.SphereElems)
            {
                json S;
                S["Center"] = { Sphere.Center.X, Sphere.Center.Y, Sphere.Center.Z };
                S["Radius"] = Sphere.Radius;
                BodyJson["Spheres"].push_back(S);
            }

            // Box 저장
            for (const FKBoxElem& Box : Body->AggGeom.BoxElems)
            {
                json B;
                B["Center"] = { Box.Center.X, Box.Center.Y, Box.Center.Z };
                B["Extent"] = { Box.Extent.X, Box.Extent.Y, Box.Extent.Z };
                FRotator Rotator = Box.Rotation;
                B["Rotation"] = { Rotator.Roll, Rotator.Pitch, Rotator.Yaw };
                BodyJson["Boxes"].push_back(B);
            }

            Root["Bodies"].push_back(BodyJson);
        }

        for (const UPhysicsConstraintTemplate* Constraint : PhysAsset->ConstraintSetup)
        {
            if (!Constraint) continue;
            const FConstraintInstance& Inst = Constraint->DefaultInstance;

            json C;
            C["JointName"] = (*Inst.JointName.ToString());
            C["Bone1"] = (*Inst.ConstraintBone1.ToString());
            C["Bone2"] = (*Inst.ConstraintBone2.ToString());
            C["Swing1"] = Inst.ProfileInstance.ConeLimit.Swing1LimitDegrees;
            C["Swing2"] = Inst.ProfileInstance.ConeLimit.Swing2LimitDegrees;
            C["Twist"] = Inst.ProfileInstance.TwistLimit.TwistLimitDegrees;

            Root["Constraints"].push_back(C);
        }

        std::ofstream OutFile((*FilePath));
        OutFile << Root.dump(4);
    }
    bool LoadPhysicsSettings(USkeletalMesh* SkeletalMesh, const FString& MeshPath)
    {
        if (!SkeletalMesh || !SkeletalMesh->GetPhysicsAsset()) return false;

        UPhysicsAsset* PhysAsset = SkeletalMesh->GetPhysicsAsset();
        FString FilePath = MeshPath + TEXT("_PhysicsSettings.json");

        std::ifstream InFile((*FilePath));
        if (!InFile.is_open()) return false;

        nlohmann::json Root;
        try
        {
            InFile >> Root;
        }
        catch (...)
        {
            return false;
        }

        for (const auto& BodyJson : Root["Bodies"])
        {
            FName BoneName((BodyJson["BoneName"].get<std::string>().c_str()));
            int32 BodyIndex = PhysAsset->FindBodyIndex(BoneName);
            if (BodyIndex == INDEX_NONE) continue;

            UBodySetup* Body = PhysAsset->BodySetup[BodyIndex];
            if (!Body) continue;

            Body->AggGeom.SphylElems.Empty();
            if (BodyJson.contains("Capsules"))
                for (const auto& Capsule : BodyJson["Capsules"])
                {
                    FKSphylElem Elem;
                    Elem.Center = FVector(Capsule["Center"][0], Capsule["Center"][1], Capsule["Center"][2]);
                    Elem.RQuat = FQuat(Capsule["Rotation"][0], Capsule["Rotation"][1], Capsule["Rotation"][2], Capsule["Rotation"][3]);
                    Elem.Radius = Capsule["Radius"];
                    Elem.Length = Capsule["Length"];
                    Body->AggGeom.SphylElems.Add(Elem);
                }
            Body->AggGeom.SphereElems.Empty();
            if (BodyJson.contains("Spheres"))
                for (const auto& S : BodyJson["Spheres"])
                {
                    FKSphereElem Elem;
                    Elem.Center = FVector(S["Center"][0], S["Center"][1], S["Center"][2]);
                    Elem.Radius = S["Radius"];
                    Body->AggGeom.SphereElems.Add(Elem);
                }

            Body->AggGeom.BoxElems.Empty();
            if (BodyJson.contains("Boxes"))
                for (const auto& B : BodyJson["Boxes"])
                {
                    FKBoxElem Elem;
                    Elem.Center = FVector(B["Center"][0], B["Center"][1], B["Center"][2]);
                    Elem.Extent = FVector(B["Extent"][0], B["Extent"][1], B["Extent"][2]);
                    Elem.Rotation = FRotator(B["Rotation"][0], B["Rotation"][1], B["Rotation"][2]);
                    Body->AggGeom.BoxElems.Add(Elem);
                }

        }

        for (const auto& C : Root["Constraints"])
        {
            FName JointName((C["JointName"].get<std::string>().c_str()));
            for (UPhysicsConstraintTemplate* Constraint : PhysAsset->ConstraintSetup)
            {
                if (!Constraint || Constraint->DefaultInstance.JointName != JointName) continue;

                Constraint->DefaultInstance.ProfileInstance.ConeLimit.Swing1LimitDegrees = C["Swing1"];
                Constraint->DefaultInstance.ProfileInstance.ConeLimit.Swing2LimitDegrees = C["Swing2"];
                Constraint->DefaultInstance.ProfileInstance.TwistLimit.TwistLimitDegrees = C["Twist"];
            }
        }

        return true;
    }

}
