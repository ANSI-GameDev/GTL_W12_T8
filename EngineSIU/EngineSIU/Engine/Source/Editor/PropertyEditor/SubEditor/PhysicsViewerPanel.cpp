#include "PhysicsViewerPanel.h"

#include "ReferenceSkeleton.h"
#include "UnrealClient.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/EnumClassFlags.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

void PhysicsViewerPanel::Render()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height));

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("PhysicsViewer", nullptr, windowFlags))
    {

        RenderPanelLayout();
        /*
        RenderViewportPanel();
        ImGui::Separator();
        RenderPhysicsSettings();
        ImGui::Separator();
        RenderInfoPanel();
        RenderSkeletonUI();*/
        ImGui::End();
    }
}


void PhysicsViewerPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    if (hWnd && GetClientRect(hWnd, &clientRect))
    {
        Width = static_cast<float>(clientRect.right - clientRect.left);
        Height = static_cast<float>(clientRect.bottom - clientRect.top);
    }
    if (ViewportClient)
    {
        FViewport* Viewport = ViewportClient->GetViewport();
        if (Viewport)
        {
            FRect NewRect(0,0,Width,Height);
            Viewport->ResizeViewport(NewRect);
        }
    }
}
void PhysicsViewerPanel::SetViewportClient(std::shared_ptr<FEditorViewportClient> InViewportClient)
{
    ViewportClient = InViewportClient;
}

void PhysicsViewerPanel::SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
{
    SkeletalMeshComponent = InSkeletalMeshComponent;
    SkeletalMeshComponent->SetSelectedBone(-1);
}

void PhysicsViewerPanel::RenderViewportPanel()
{
    if (!ViewportClient) return;

    FViewport* Viewport = ViewportClient->GetViewport();
    if (!Viewport) return;

    FViewportResource* Resource = Viewport->GetViewportResource();
    if (!Resource) return;

    FRenderTargetRHI* RenderTarget = Resource->GetRenderTarget(EResourceType::ERT_Compositing);
    if (RenderTarget && RenderTarget->SRV)
    {
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(RenderTarget->SRV), contentSize);
    }
 }

void PhysicsViewerPanel::RenderPhysicsSettings()
{
    /*ImGui::Text("Physics Settings:");
    ImGui::Checkbox("Enable Gravity", /* TODO: Hook to simulation setting #1# nullptr);
    ImGui::SliderFloat("Time Step", /* TODO: Hook to sim delta #1# nullptr, 0.001f, 0.033f);*/
}

void PhysicsViewerPanel::RenderInfoPanel()
{
    /*ImGui::Text("Debug Info:");
    ImGui::Text("Body Count: %d", /* TODO: replace with actual data #1# 0);
    ImGui::Text("Broadphase Type: %s", /* TODO: #1# "AABB Grid");*/
}
void PhysicsViewerPanel::RenderBoneRecursive(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, FBaseCompactPose& Pose)
{
    ImGui::PushID(BoneIndex);
    const FName& BoneName = RefSkeleton.GetBoneName(BoneIndex);

    // 현재 Bone의 트리 노드 생성
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    bool bOpen = ImGui::TreeNodeEx(*BoneName.ToString(), flags);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        if (SkeletalMeshComponent)
        {
            SkeletalMeshComponent->SetSelectedBone(BoneIndex);
            SelectedType = EPhysicsSelectionType::Bone;
            SelectedName = BoneName;
        }
    }

    if (bOpen)
    {
        // [1] 자식 Bone 재귀 호출
        for (int32 i = 0; i < RefSkeleton.GetRawBoneNum(); ++i)
        {
            if (RefSkeleton.GetParentIndex(i) == BoneIndex)
            {
                RenderBoneRecursive(RefSkeleton, i, Pose);
            }
        }

        UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetPhysicsAsset();

        if (PhysicsAsset)
        {
            // [2] 현재 Bone에 연결된 Constraint들 출력
            if (EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Constraint))
            {
                for (int32 i = 0; i < PhysicsAsset->ConstraintSetup.Num(); ++i)
                {
                    const UPhysicsConstraintTemplate* Constraint = PhysicsAsset->ConstraintSetup[i];
                    if (!Constraint) continue;

                    const FConstraintInstance& Inst = Constraint->DefaultInstance;

                    if (Inst.ConstraintBone1 == BoneName/* || Inst.ConstraintBone2 == BoneName*/)
                    {
                        FString CName = Inst.JointName.ToString() + TEXT(" [Constraint]");
                        bool bClicked = ImGui::TreeNodeEx(*CName, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                        if (ImGui::IsItemClicked())
                        {
                            SelectedType = EPhysicsSelectionType::Constraint;
                            SelectedName = Inst.JointName;
                        }
                    }
                }
            }

            // [3] 현재 Bone이 Body를 가지는 경우 출력
            if (EnumHasAnyFlags(DebugDisplayFlags, EPhysicsDebugDisplay::Body))
            {
                if (PhysicsAsset->FindBodyIndex(BoneName) != INDEX_NONE)
                {
                    FString BodyLabel = BoneName.ToString() + TEXT(" [Body]");
                    ImGui::TreeNodeEx(*BodyLabel, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                    if (ImGui::IsItemClicked())
                    {
                        SelectedType = EPhysicsSelectionType::Body;
                        SelectedName = BoneName;
                    }
                }
            }
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}



inline void PhysicsViewerPanel::RenderSkeletonUI()
{
    if (!SkeletalMeshComponent || !SkeletalMeshComponent->GetSkeletalMeshAsset())
        return;

    const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetRefSkeleton();
    if (RefSkeleton.GetRawBoneNum() == 0)
        return;

    float rightW = Width * 0.25f;
    float panelX = Width - rightW;
    float panelY = 0.0f;

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightW, Height), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Skeleton Hierarchy", nullptr, flags))
    {
        FBaseCompactPose& Pose = SkeletalMeshComponent->BonePoseContext.Pose;

        const auto& SkeletalMeshes = UAssetManager::Get().GetSkeletalMeshMap();
        static int CurrentIndex = -1;
        static TArray<FName> MeshNames;
        MeshNames.Empty();
        for (auto& Pair : SkeletalMeshes) MeshNames.Add(Pair.Key);

        if (ImGui::BeginCombo("SkeletalMesh", (CurrentIndex >= 0 && MeshNames.IsValidIndex(CurrentIndex)) ? *MeshNames[CurrentIndex].ToString() : "None"))
        {
            for (int i = 0; i < MeshNames.Num(); ++i)
            {
                bool bSelected = (i == CurrentIndex);
                if (ImGui::Selectable(*MeshNames[i].ToString(), bSelected))
                {
                    CurrentIndex = i;
                    SkeletalMeshComponent->SetSkeletalMeshAsset(UAssetManager::Get().GetSkeletalMesh(MeshNames[i]));
                }
                if (bSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetRawBoneNum(); ++BoneIndex)
        {
            if (RefSkeleton.GetParentIndex(BoneIndex) == INDEX_NONE)
            {
                RenderBoneRecursive(RefSkeleton, BoneIndex, Pose);
            }
        }

        RenderSelectedProperty(Pose);
        /*
        int SelectedBoneIndex = SkeletalMeshComponent->GetSelectedBone();
        if (SelectedBoneIndex != INDEX_NONE && Pose.IsValidIndex(SelectedBoneIndex))
        {
            ImGui::SeparatorText("Bone Transform");

            // 편집할 Transform 복사
            FTransform BoneTransform = Pose.GetBoneTransform(SelectedBoneIndex);
            FVector Translation = BoneTransform.GetTranslation();
            FQuat RotationQuat = BoneTransform.GetRotation();
            FRotator Rotator = RotationQuat.Rotator();

            static int32 LastBoneIndex = -1;
            static float EulerAngles[3] = { 0.f, 0.f, 0.f };

            // 본이 바뀌면 회전값 초기화
            if (SelectedBoneIndex != LastBoneIndex)
            {
                EulerAngles[0] = Rotator.Roll;
                EulerAngles[1] = Rotator.Yaw;
                EulerAngles[2] = Rotator.Pitch;
                LastBoneIndex = SelectedBoneIndex;
            }

            float pos[3] = { Translation.X, Translation.Y, Translation.Z };
            bool bChanged = false;

            if (ImGui::DragFloat3("Position", pos, 0.1f))
            {
                BoneTransform.SetTranslation(FVector(pos[0], pos[1], pos[2]));
                bChanged = true;
            }

            if (ImGui::DragFloat3("Rotation", EulerAngles, 0.5f))
            {
                FRotator NewRotator(EulerAngles[0], EulerAngles[1], EulerAngles[2]); // Roll, Yaw,Pitch  순서

                BoneTransform.SetRotation(FQuat(NewRotator));
                bChanged = true;
            }

            if (bChanged)
            {
                Pose.SetBoneTransform(SelectedBoneIndex, BoneTransform);
            }
        }
        */

    }

    ImGui::End();
}

void PhysicsViewerPanel::RenderSelectedProperty(FBaseCompactPose& Pose)
{
    if (!SkeletalMeshComponent) return;

    UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetPhysicsAsset();

    if (SelectedType == EPhysicsSelectionType::Bone)
    {
        const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetRefSkeleton();
        int32 BoneIndex = RefSkeleton.FindBoneIndex(SelectedName);
        if (BoneIndex == INDEX_NONE || !Pose.IsValidIndex(BoneIndex)) return;

        ImGui::SeparatorText("Bone Transform");

        FTransform BoneTransform = Pose.GetBoneTransform(BoneIndex);
        FVector Translation = BoneTransform.GetTranslation();
        FRotator Rotator = BoneTransform.GetRotation().Rotator();

        static int32 LastBoneIndex = -1;
        static float EulerAngles[3] = { 0.f, 0.f, 0.f };

        if (LastBoneIndex != BoneIndex)
        {
            // Bone이 바뀐 경우에만 회전값 초기화
            EulerAngles[0] = Rotator.Roll;
            EulerAngles[1] = Rotator.Yaw;
            EulerAngles[2] = Rotator.Pitch;
            LastBoneIndex = BoneIndex;
        }

        float pos[3] = { Translation.X, Translation.Y, Translation.Z };
        bool bChanged = false;

        if (ImGui::DragFloat3("Position", pos, 0.1f))
        {
            BoneTransform.SetTranslation(FVector(pos[0], pos[1], pos[2]));
            bChanged = true;
        }

        if (ImGui::DragFloat3("Rotation", EulerAngles, 0.5f))
        {
            BoneTransform.SetRotation(FQuat(FRotator(EulerAngles[0], EulerAngles[1], EulerAngles[2])));
            bChanged = true;
        }

        if (bChanged)
        {
            Pose.SetBoneTransform(BoneIndex, BoneTransform);
        }
    }


    else if (SelectedType == EPhysicsSelectionType::Body)
    {
        if (!PhysicsAsset) return;

        int32 BodyIndex = PhysicsAsset->FindBodyIndex(SelectedName);
        if (BodyIndex == INDEX_NONE) return;

        ImGui::SeparatorText("Body Settings");
        
        UBodySetup* BodySetup = PhysicsAsset->BodySetup[BodyIndex];
    }
    else if (SelectedType == EPhysicsSelectionType::Constraint)
    {
        if (!PhysicsAsset) return;

        for (UPhysicsConstraintTemplate* Constraint : PhysicsAsset->ConstraintSetup)
        {
            if (Constraint && Constraint->DefaultInstance.JointName == SelectedName)
            {
                ImGui::SeparatorText("Constraint Settings");

                FConstraintInstance& Inst = Constraint->DefaultInstance;

                // [1] Linear Constraint
                ImGui::Text("Linear Constraint");

                float LinearLimit = Inst.ProfileInstance.LinearLimit.Limit;
                if (ImGui::DragFloat("Limit", &LinearLimit, 0.1f, 0.0f, 1000.0f))
                    Inst.ProfileInstance.LinearLimit.Limit = LinearLimit;

                const char* LinearMotionTypes[] = { "Free", "Limited", "Locked" };

                int xMotion = Inst.ProfileInstance.LinearLimit.XMotion;
                int yMotion = Inst.ProfileInstance.LinearLimit.YMotion;
                int zMotion = Inst.ProfileInstance.LinearLimit.ZMotion;

                ImGui::Combo("X Motion", &xMotion, LinearMotionTypes, IM_ARRAYSIZE(LinearMotionTypes));
                ImGui::Combo("Y Motion", &yMotion, LinearMotionTypes, IM_ARRAYSIZE(LinearMotionTypes));
                ImGui::Combo("Z Motion", &zMotion, LinearMotionTypes, IM_ARRAYSIZE(LinearMotionTypes));

                Inst.ProfileInstance.LinearLimit.XMotion = static_cast<ELinearConstraintMotion>(xMotion);
                Inst.ProfileInstance.LinearLimit.YMotion = static_cast<ELinearConstraintMotion>(yMotion);
                Inst.ProfileInstance.LinearLimit.ZMotion = static_cast<ELinearConstraintMotion>(zMotion);

                // [2] Cone (Swing) Constraint
                ImGui::Separator();
                ImGui::Text("Swing (Cone) Constraint");

                float Swing1 = Inst.ProfileInstance.ConeLimit.Swing1LimitDegrees;
                float Swing2 = Inst.ProfileInstance.ConeLimit.Swing2LimitDegrees;

                if (ImGui::DragFloat("Swing1 Limit (Y)", &Swing1, 0.1f, 0.0f, 180.0f))
                    Inst.ProfileInstance.ConeLimit.Swing1LimitDegrees = Swing1;

                if (ImGui::DragFloat("Swing2 Limit (X)", &Swing2, 0.1f, 0.0f, 180.0f))
                    Inst.ProfileInstance.ConeLimit.Swing2LimitDegrees = Swing2;

                int swing1Motion = Inst.ProfileInstance.ConeLimit.Swing1Motion;
                int swing2Motion = Inst.ProfileInstance.ConeLimit.Swing2Motion;
                const char* AngularMotionTypes[] = { "Free", "Limited", "Locked" };

                ImGui::Combo("Swing1 Motion", &swing1Motion, AngularMotionTypes, IM_ARRAYSIZE(AngularMotionTypes));
                ImGui::Combo("Swing2 Motion", &swing2Motion, AngularMotionTypes, IM_ARRAYSIZE(AngularMotionTypes));

                Inst.ProfileInstance.ConeLimit.Swing1Motion = static_cast<EAngularConstraintMotion>(swing1Motion);
                Inst.ProfileInstance.ConeLimit.Swing2Motion = static_cast<EAngularConstraintMotion>(swing2Motion);

                // [3] Twist Constraint
                ImGui::Separator();
                ImGui::Text("Twist Constraint");

                float Twist = Inst.ProfileInstance.TwistLimit.TwistLimitDegrees;
                if (ImGui::DragFloat("Twist Limit (Z)", &Twist, 0.1f, 0.0f, 180.0f))
                    Inst.ProfileInstance.TwistLimit.TwistLimitDegrees = Twist;

                int twistMotion = Inst.ProfileInstance.TwistLimit.TwistMotion;
                ImGui::Combo("Twist Motion", &twistMotion, AngularMotionTypes, IM_ARRAYSIZE(AngularMotionTypes));
                Inst.ProfileInstance.TwistLimit.TwistMotion = static_cast<EAngularConstraintMotion>(twistMotion);

                // [4] 연결 정보
                ImGui::Separator();
                ImGui::Text("Connected Bones:");
                ImGui::BulletText("Bone1: %s", *Inst.ConstraintBone1.ToString());
                ImGui::BulletText("Bone2: %s", *Inst.ConstraintBone2.ToString());

                break;
            }
        }
    }


}

inline void PhysicsViewerPanel::RenderPanelLayout()
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = Width * 0.75f;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftW, Height), ImGuiCond_Always);

    ImGuiWindowFlags canvasFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    if (ImGui::Begin("PhysicsCanvas", nullptr, canvasFlags))
    {
        RenderViewportPanel();
        ImGui::Separator();
        RenderPhysicsSettings();
        ImGui::Separator();
        RenderInfoPanel();
    }
    ImGui::End();

    RenderSkeletonUI();
}
