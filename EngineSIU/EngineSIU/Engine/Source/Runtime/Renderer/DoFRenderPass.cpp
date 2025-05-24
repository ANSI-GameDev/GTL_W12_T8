#include "DoFRenderPass.h"

#include "RendererHelpers.h"
#include "ShaderConstants.h"
#include "UnrealClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "LevelEditor/SLevelEditor.h"
#include "PropertyEditor/ShowFlags.h"
#include "UnrealEd/EditorViewportClient.h"

FDoFRenderPass::FDoFRenderPass()
{
}

FDoFRenderPass::~FDoFRenderPass()
{
    ReleaseShader();
}

void FDoFRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    ShaderManager = InShaderManager;
    CreateShader();
    CreateSampler();
}

void FDoFRenderPass::PrepareRenderArr()
{
}

void FDoFRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    const EViewModeIndex ViewMode = Viewport->GetViewMode();
    if (ViewMode == EViewModeIndex::VMI_Wireframe || !(ShowFlag & static_cast<uint64>(EEngineShowFlags::SF_DepthOfField)))
    {
        return;
    }

    BufferManager->BindConstantBuffer("DofBlurConstants", 0, EShaderStage::Pixel);
    
    float focusDistance = Viewport->FDistance;
    FDoFBlurConstants constants;
    constants.TexelSize.X = 1/Viewport->Viewport->GetRect().Width;
    constants.TexelSize.Y = 1/Viewport->Viewport->GetRect().Height;
    constants.blurScale = 1.0f;
    constants.focusDistance = Viewport->FDistance;
    constants.focusDepth = Viewport->FDepth;
    constants.nearClip = Viewport->NearClip;
    constants.farClip = Viewport->FarClip;

    BufferManager->UpdateConstantBuffer<FDoFBlurConstants>("DofBlurConstants", constants);

    PrepareRenderState();

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* SceneRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_Scene);
    FDepthStencilRHI* DepthRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);
    FRenderTargetRHI* BlurTempRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_PP_BlurTemp);

    Graphics->DeviceContext->PSSetShaderResources(static_cast<uint8>(EShaderSRVSlot::SRV_Scene), 1, &SceneRHI->SRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<uint8>(EShaderSRVSlot::SRV_SceneDepth), 1, &DepthRHI->SRV);
    Graphics->DeviceContext->OMSetRenderTargets(1, &BlurTempRHI->RTV, nullptr);
    UpdateHShader();
    Graphics->DeviceContext->Draw(6, 0);
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    
    Graphics->DeviceContext->PSSetShaderResources(static_cast<uint8>(EShaderSRVSlot::SRV_Scene), 1, &BlurTempRHI->SRV);
    Graphics->DeviceContext->OMSetRenderTargets(1, &SceneRHI->RTV, nullptr);
    UpdateVShader();
    Graphics->DeviceContext->Draw(6, 0);
    
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FDoFRenderPass::ClearRenderArr()
{
}

void FDoFRenderPass::PrepareRenderState()
{
    Graphics->DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    Graphics->DeviceContext->IASetInputLayout(nullptr);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    
    Graphics->DeviceContext->PSSetSamplers(0, 1, &Sampler);
}

void FDoFRenderPass::CreateShader()
{
    HRESULT hr;
    hr = ShaderManager->AddVertexShader(L"DoFVertexShader", L"Shaders/DofEffectShader.hlsl", "mainVS");
    if (FAILED(hr))
    {
        return;
    }
    hr = ShaderManager->AddPixelShader(L"DofPixelHShader", L"Shaders/DofEffectShader.hlsl", "PS_BlurH");
    if (FAILED(hr))
    {
        return;
    }
    hr = ShaderManager->AddPixelShader(L"DofPixelVShader", L"Shaders/DofEffectShader.hlsl", "PS_BlurV");
    if (FAILED(hr))
    {
        return;
    }
    VertexShader = ShaderManager->GetVertexShaderByKey(L"DoFVertexShader");
    PixelHShader = ShaderManager->GetPixelShaderByKey(L"DofPixelHShader");
    PixelVShader = ShaderManager->GetPixelShaderByKey(L"DofPixelVShader");
    BufferManager->CreateBufferGeneric<FDoFBlurConstants>("DofBlurConstants", nullptr, sizeof(FDoFBlurConstants), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void FDoFRenderPass::UpdateHShader()
{
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelHShader, nullptr, 0);
}

void FDoFRenderPass::UpdateVShader()
{
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelVShader, nullptr, 0);
}

void FDoFRenderPass::ReleaseShader()
{
}

void FDoFRenderPass::CreateSampler()
{
    D3D11_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDesc.MinLOD = 0;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    Graphics->Device->CreateSamplerState(&SamplerDesc, &Sampler);
}
