#include "PhysicsViewerPanel.h"

#include "ReferenceSkeleton.h"
#include "UnrealClient.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

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
}
void PhysicsViewerPanel::SetViewportClient(std::shared_ptr<FEditorViewportClient> InViewportClient)
{
    ViewportClient = InViewportClient;
}

void PhysicsViewerPanel::SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
{
    SkeletalMeshComponent = InSkeletalMeshComponent;
    SelectedBoneIndex = -1;
}

void PhysicsViewerPanel::RenderViewportPanel()
{
    if (!ViewportClient) return;

    FViewport* Viewport = ViewportClient->GetViewport();
    if (!Viewport) return;

    FViewportResource* Resource = Viewport->GetViewportResource();
    if (!Resource) return;

    FRenderTargetRHI* RenderTarget = Resource->GetRenderTarget(EResourceType::ERT_Scene);
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

    bool bHasChildren = false;
    for (int32 i = 0; i < RefSkeleton.GetRawBoneNum(); ++i)
    {
        if (RefSkeleton.GetParentIndex(i) == BoneIndex)
        {
            bHasChildren = true;
            break;
        }
    }

    ImGuiTreeNodeFlags flags = bHasChildren ? ImGuiTreeNodeFlags_OpenOnArrow : (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    bool bOpen = ImGui::TreeNodeEx(*BoneName.ToString(), flags);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        SelectedBoneIndex = BoneIndex;
    }

    if (bOpen && bHasChildren)
    {
        for (int32 i = 0; i < RefSkeleton.GetRawBoneNum(); ++i)
        {
            if (RefSkeleton.GetParentIndex(i) == BoneIndex)
            {
                RenderBoneRecursive(RefSkeleton, i, Pose);
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

    const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetReferenceSkeleton();
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

    }

    ImGui::End();
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
