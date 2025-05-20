// ParticleRenderPass.cpp (Instancing Version)
#include "ParticleRenderPass.h"

#include "EngineLoop.h"
#include "Particles/ParticleHelper.h"
#include "Engine/Engine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "UnrealClient.h"
#include "RendererHelpers.h"
#include "Components/ParticleSystemComponent.h"
#include "Engine/FObjLoader.h"
#include "Engine/StaticMesh.h"
#include "Engine/Asset/StaticMeshAsset.h"
#include "Particles/ParticleEmitterInstances.h"
#include "UObject/UObjectIterator.h"

struct FSpriteVertex
{
    FVector2D UV;
};

struct FSpriteParticleInstance
{
    FVector Position;
    float RelativeTime;
    FVector OldPosition;
    float ParticleId;
    FVector2D Size;
    float Rotation;
    float SubImageIndex;
    FLinearColor Color;
};
struct FMeshParticleInstance
{
    FMatrix Transform;      // 전체 4x4 행렬
    FLinearColor Color;
};

FParticleRenderPass::FParticleRenderPass()
    : BufferManager(nullptr), Graphics(nullptr), ShaderManager(nullptr)
{
}

FParticleRenderPass::~FParticleRenderPass()
{
    ReleaseShader();
}

void FParticleRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateShader();

    // Create static quad vertex buffer (6 vertices)
    TArray<FSpriteVertex> QuadVertices = {
        { {-0.5f, -0.5f} }, { {0.5f, -0.5f} }, { {0.5f, 0.5f} },
        { {-0.5f, -0.5f} }, { {0.5f, 0.5f} }, { {-0.5f, 0.5f} },
    };
    BufferManager->CreateVertexBuffer(TEXT("QuadSpriteVertex"), QuadVertices, QuadVertexInfo);

    BufferManager->CreateDynamicVertexBuffer(
        TEXT("Global_SpriteInstance"), 
        sizeof(FSpriteParticleInstance) * 1000, 
        InstanceInfoSprite
    );
    BufferManager->CreateDynamicVertexBuffer(
        TEXT("Global_MeshInstance"),
        sizeof(FMeshParticleInstance) * 1000,
        InstanceInfoMesh
    );

}

void FParticleRenderPass::CreateShader()
{
    // Sprite용
    D3D_SHADER_MACRO DefinesSprite[] = {
        { "PARTICLE_SPRITE", "1" },
        { nullptr, nullptr }
    };
    D3D11_INPUT_ELEMENT_DESC InputLayout[] = {
        { "TEXCOORD",        0, DXGI_FORMAT_R32G32_FLOAT,        0, 0,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "INSTANCE_POS",    0, DXGI_FORMAT_R32G32B32_FLOAT,     1, 0,   D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_TIME",   0, DXGI_FORMAT_R32_FLOAT,           1, 12,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_OLDPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT,     1, 16,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_ID",     0, DXGI_FORMAT_R32_FLOAT,           1, 28,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_SIZE",   0, DXGI_FORMAT_R32G32_FLOAT,        1, 32,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_ROT",    0, DXGI_FORMAT_R32_FLOAT,           1, 40,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_SUBUV",  0, DXGI_FORMAT_R32_FLOAT,           1, 44,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_COLOR",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, 48,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };
    ShaderManager->AddVertexShaderAndInputLayout(L"ParticleShader_Sprite", L"Shaders/ParticleShader.hlsl", "mainVS", InputLayout, ARRAYSIZE(InputLayout), DefinesSprite);
    ShaderManager->AddPixelShader(L"ParticleShader_Sprite", L"Shaders/ParticleShader.hlsl", "mainPS", DefinesSprite);

    // Mesh용
    D3D_SHADER_MACRO DefinesMesh[] = {
        { "PARTICLE_MESH", "1" },
        { nullptr, nullptr }
    };
    D3D11_INPUT_ELEMENT_DESC InputLayoutMesh[] = {
        { "POSITION",           0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD",           0, DXGI_FORMAT_R32G32_FLOAT,        0, 56, D3D11_INPUT_PER_VERTEX_DATA,   0 }, // UV
        { "INSTANCE_TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };


    ShaderManager->AddVertexShaderAndInputLayout(L"ParticleShader_Mesh", L"Shaders/ParticleShader.hlsl", "mainVS", InputLayoutMesh, ARRAYSIZE(InputLayoutMesh), DefinesMesh);
    ShaderManager->AddPixelShader(L"ParticleShader_Mesh", L"Shaders/ParticleShader.hlsl", "mainPS", DefinesMesh);
}


void FParticleRenderPass::ReleaseShader() {}

void FParticleRenderPass::PrepareRenderArr()
{
    ParticleComponents.Empty();
    for (auto Comp : TObjectRange<UParticleSystemComponent>())
    {
        if (Comp->GetWorld() == GEngine->ActiveWorld)
        {
            ParticleComponents.Add(Comp);
        }
    }
}

void FParticleRenderPass::PrepareRenderState(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    /*ID3D11VertexShader* VS = ShaderManager->GetVertexShaderByKey(L"ParticleShader");
    ID3D11InputLayout* IL = ShaderManager->GetInputLayoutByKey(L"ParticleShader");
    ID3D11PixelShader* PS = ShaderManager->GetPixelShaderByKey(L"ParticleShader");

    Graphics->DeviceContext->VSSetShader(VS, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(IL);
    Graphics->DeviceContext->PSSetShader(PS, nullptr, 0);*/
    BufferManager->BindConstantBuffer(TEXT("FParticleSettingsConstants"), 4, EShaderStage::Vertex);

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RT = ViewportResource->GetRenderTarget(EResourceType::ERT_Scene);
    FDepthStencilRHI* DS = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);
    Graphics->DeviceContext->RSSetViewports(1, &ViewportResource->GetD3DViewport());
    Graphics->DeviceContext->OMSetRenderTargets(1, &RT->RTV, DS->DSV);
}
void FParticleRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRenderState(Viewport);

    for (UParticleSystemComponent* Comp : ParticleComponents)
    {
        if (!Comp || Comp->EmitterInstances.IsEmpty()) continue;

        for (FParticleEmitterInstance* Emitter : Comp->EmitterInstances)
        {
            if (!Emitter) continue;

            FDynamicEmitterDataBase* DynamicData = Emitter->GetDynamicData();
            if (!DynamicData) continue;

            const FDynamicEmitterReplayDataBase& ReplayData = DynamicData->GetSource();
            switch (ReplayData.eEmitterType)
            {
            case DET_Sprite:
                RenderMeshEmitter(Comp, Emitter, (const FDynamicMeshEmitterReplayDataBase&)ReplayData);
                //RenderSpriteEmitter(Comp, Emitter, (const FDynamicSpriteEmitterReplayDataBase&)ReplayData);
                break;

            case DET_Mesh:
                RenderMeshEmitter(Comp, Emitter, (const FDynamicMeshEmitterReplayDataBase&)ReplayData);
                break;

            default:
                break;
            }
        }
    }

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}
void FParticleRenderPass::RenderSpriteEmitter(UParticleSystemComponent* Comp, FParticleEmitterInstance* Emitter, const FDynamicSpriteEmitterReplayDataBase& ReplayData)
{
    //TestTexture = FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/T_Explosion_SubUV.png");
    ID3D11VertexShader* VS = ShaderManager->GetVertexShaderByKey(L"ParticleShader_Sprite");
    ID3D11InputLayout* IL = ShaderManager->GetInputLayoutByKey(L"ParticleShader_Sprite");
    ID3D11PixelShader* PS = ShaderManager->GetPixelShaderByKey(L"ParticleShader_Sprite");

    Graphics->DeviceContext->VSSetShader(VS, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(IL);
    Graphics->DeviceContext->PSSetShader(PS, nullptr, 0);

    static float SubImageIndex = 0.0f;
    static float SubImageIndexTimer = 0.0f;
    SubImageIndexTimer += 0.05f;
    if (SubImageIndexTimer >= 1.0f)
    {
        SubImageIndexTimer = 0.0f;
        SubImageIndex += 1.0f;
        if (SubImageIndex >= 36) SubImageIndex = 0.0f;
    }

    TArray<FSpriteParticleInstance> Instances;

    const int32 Stride = Emitter->ParticleStride;
    const int32 ActiveCount = Emitter->ActiveParticles;

    for (int32 i = 0; i < ActiveCount; ++i)
    {
        const int32 ParticleIndex = Emitter->ParticleIndices[i];
        const FBaseParticle* P = reinterpret_cast<const FBaseParticle*>(Emitter->ParticleData + ParticleIndex * Stride);

        //TODO: ParticleData에서 SubUV 정보 가져오기
        //Inst도 다른 곳에서 가져와야 함
        FSpriteParticleInstance Inst;
        Inst.Position = P->Location;
        Inst.OldPosition = P->OldLocation;
        Inst.RelativeTime = P->RelativeTime;
        Inst.ParticleId = static_cast<float>(i);
        Inst.Size = FVector2D(P->Size.X, P->Size.Y);
        Inst.Rotation = P->Rotation;
        Inst.SubImageIndex = SubImageIndex;
        Inst.Color = P->Color;
        Instances.Add(Inst);
    }

    if (Instances.IsEmpty()) return;

    // Upload instance buffer
    BufferManager->UpdateDynamicVertexBuffer(TEXT("Global_SpriteInstance"), Instances);

    // Set Object Constant Buffer
    FObjectConstantBuffer ObjectData;
    FMatrix WorldMatrix = Comp->GetWorldMatrix();
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = Comp->EncodeUUID() / 255.0f;
    ObjectData.bIsSelected = false;
    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);

    // SubUV 정보 바인딩
    FParticleSettingsConstants ParticleSettings;
    ParticleSettings.SubUVCols = 6;
    ParticleSettings.SubUVRows = 6;
    BufferManager->UpdateConstantBuffer("FParticleSettingsConstants", ParticleSettings);

    // Draw
    ID3D11Buffer* Buffers[2] = { QuadVertexInfo.VertexBuffer, InstanceInfoSprite.VertexBuffer };
    UINT Strides[2] = { sizeof(FSpriteVertex), sizeof(FSpriteParticleInstance) };
    UINT Offsets[2] = { 0, 0 };

    //Sprite니까 무조건 0번?
    std::shared_ptr<FTexture> Texture = 
        FEngineLoop::ResourceManager.GetTexture(ReplayData.Material->GetMaterialInfo().TextureInfos[0].TexturePath);

    ID3D11ShaderResourceView* SRVs[9] = {};
    ID3D11SamplerState* Samplers[9] = {};
    if (ReplayData.Material->GetMaterialInfo().TextureInfos.IsValidIndex(0))
    {
        const FWString& TexturePath = ReplayData.Material[0].GetMaterialInfo().TextureInfos[0].TexturePath;
        std::shared_ptr<FTexture> Texture = FEngineLoop::ResourceManager.GetTexture(TexturePath);
        if (Texture)
        {
            SRVs[0] = Texture->TextureSRV;
            Samplers[0] = Texture->SamplerState;
        }
    }

    Graphics->DeviceContext->PSSetShaderResources(0, 1, &Texture->TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &Texture->SamplerState);
    Graphics->DeviceContext->IASetVertexBuffers(0, 2, Buffers, Strides, Offsets);
    Graphics->DeviceContext->DrawInstanced(6, Instances.Num(), 0, 0);
}
void FParticleRenderPass::RenderMeshEmitter(UParticleSystemComponent* Comp, FParticleEmitterInstance* Emitter, const FDynamicMeshEmitterReplayDataBase& ReplayData)
{
    ID3D11VertexShader* VS = ShaderManager->GetVertexShaderByKey(L"ParticleShader_Mesh");
    ID3D11InputLayout* IL = ShaderManager->GetInputLayoutByKey(L"ParticleShader_Mesh");
    ID3D11PixelShader* PS = ShaderManager->GetPixelShaderByKey(L"ParticleShader_Mesh");

    Graphics->DeviceContext->VSSetShader(VS, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(IL);
    Graphics->DeviceContext->PSSetShader(PS, nullptr, 0);


    //나중에 파티클 모듈에서 가져와야함
    UStaticMesh* Mesh = FObjManager::GetStaticMesh(TestMeshAssetName.ToString().ToWideString());
    BufferManager->CreateVertexBuffer(
        "TestMeshVertex",
        Mesh->GetRenderData()->Vertices,
        StaticMeshVertexInfo
    );
    BufferManager->CreateIndexBuffer(
        "TestMeshIndex",
        Mesh->GetRenderData()->Indices,
        StaticMeshIndexInfo
    );
    if (!Mesh || !Mesh->GetRenderData()) return;

    FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
    if (RenderData->Vertices.IsEmpty() || RenderData->Indices.IsEmpty()) return;

    // 인스턴스 데이터 생성
    TArray<FMeshParticleInstance> Instances;

    const int32 Stride = Emitter->ParticleStride;
    const int32 ActiveCount = Emitter->ActiveParticles;

    for (int32 i = 0; i < ActiveCount; ++i)
    {
        const int32 ParticleIndex = Emitter->ParticleIndices[i];
        const FBaseParticle* P = reinterpret_cast<const FBaseParticle*>(Emitter->ParticleData + ParticleIndex * Stride);

        FMatrix ParticleTransform = FMatrix::Identity;
        ParticleTransform.SetOrigin(P->Location);
        FMatrix ScaleMat = FMatrix::CreateScaleMatrix(P->Size);

        ParticleTransform *= ScaleMat;
        FMeshParticleInstance Inst;
        Inst.Transform = ParticleTransform;
        Inst.Color = P->Color;
        Instances.Add(Inst);
    }

    if (Instances.IsEmpty()) return;

    BufferManager->UpdateDynamicVertexBuffer(TEXT("Global_MeshInstance"), Instances);

    // Constant Buffer
    FObjectConstantBuffer ObjectData;
    FMatrix WorldMatrix = Comp->GetWorldMatrix();
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = Comp->EncodeUUID() / 255.0f;
    ObjectData.bIsSelected = false;
    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);

    /*// 머티리얼 → 텍스처 바인딩
    const FMaterialInfo& Mat = RenderData->Materials[0];
    const FWString& TexturePath = Mat.TextureInfos[0].TexturePath;
    std::shared_ptr<FTexture> Texture = FEngineLoop::ResourceManager.GetTexture(TexturePath);*/

    // Diffuse texture only for now
    ID3D11ShaderResourceView* SRVs[9] = {};
    ID3D11SamplerState* Samplers[9] = {};
    if (RenderData->Materials[0].TextureInfos.IsValidIndex(0))
    {
        const FWString& TexturePath = RenderData->Materials[0].TextureInfos[0].TexturePath;
        std::shared_ptr<FTexture> Texture = FEngineLoop::ResourceManager.GetTexture(TexturePath);
        if (Texture)
        {
            SRVs[0] = Texture->TextureSRV;
            Samplers[0] = Texture->SamplerState;
        }
    }

    Graphics->DeviceContext->PSSetShaderResources(0, 9, SRVs);
    Graphics->DeviceContext->PSSetSamplers(0, 9, Samplers);


    // Vertex/Index/Instance 설정
    ID3D11Buffer* Buffers[2] = { StaticMeshVertexInfo.VertexBuffer, InstanceInfoMesh.VertexBuffer };
    UINT Strides[2] = { sizeof(FStaticMeshVertex), sizeof(FMeshParticleInstance) };
    UINT Offsets[2] = { 0, 0 };

    Graphics->DeviceContext->IASetVertexBuffers(0, 2, Buffers, Strides, Offsets);
    Graphics->DeviceContext->IASetIndexBuffer(StaticMeshIndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    Graphics->DeviceContext->DrawIndexedInstanced(RenderData->Indices.Num(), Instances.Num(), 0, 0, 0);
}

void FParticleRenderPass::ClearRenderArr()
{
    ParticleComponents.Empty();
}
