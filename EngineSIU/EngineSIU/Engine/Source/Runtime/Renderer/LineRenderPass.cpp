#include "LineRenderPass.h"

#include "D3D11RHI/DXDBufferManager.h"

#include "D3D11RHI/GraphicDevice.h"

#include "D3D11RHI/DXDShaderManager.h"

#include "RendererHelpers.h"

#include "Math/JungleMath.h"

#include "EngineLoop.h"
#include "UnrealClient.h"

#include "UObject/UObjectIterator.h"

#include "UnrealEd/EditorViewportClient.h"

// 생성자/소멸자
FLineRenderPass::FLineRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
    , VertexLineShader(nullptr)
    , PixelLineShader(nullptr)
{
}

FLineRenderPass::~FLineRenderPass()
{
}

void FLineRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;
    CreateShader();
}

void FLineRenderPass::PrepareRenderArr()
{
}

void FLineRenderPass::ClearRenderArr()
{
    // 필요에 따라 내부 배열을 초기화
}

void FLineRenderPass::CreateShader()
{
    HRESULT Result = ShaderManager->AddVertexShader(L"VertexLineShader", L"Shaders/ShaderLine.hlsl", "mainVS");
    Result = ShaderManager->AddPixelShader(L"PixelLineShader", L"Shaders/ShaderLine.hlsl", "mainPS");

    VertexLineShader = ShaderManager->GetVertexShaderByKey(L"VertexLineShader");
    PixelLineShader = ShaderManager->GetPixelShaderByKey(L"PixelLineShader");
}

void FLineRenderPass::UpdateShader()
{
    VertexLineShader = ShaderManager->GetVertexShaderByKey(L"VertexLineShader");
    PixelLineShader = ShaderManager->GetPixelShaderByKey(L"PixelLineShader");
}

void FLineRenderPass::PrepareLineShader() const
{
    Graphics->DeviceContext->VSSetShader(VertexLineShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(nullptr);
    Graphics->DeviceContext->PSSetShader(PixelLineShader, nullptr, 0);

    FEngineLoop::PrimitiveDrawBatch.PrepareLineResources();
}

void FLineRenderPass::DrawLineBatch(const FLinePrimitiveBatchArgs& BatchArgs) const
{
    constexpr UINT Stride = sizeof(FSimpleVertex);
    constexpr UINT Offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &BatchArgs.VertexBuffer, &Stride, &Offset);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    constexpr UINT VertexCountPerInstance = 2;
    const UINT InstanceCount = BatchArgs.GridParam.NumGridLines + 3 +
        (BatchArgs.BoundingBoxCount * 12) +
        (BatchArgs.ConeCount * (2 * BatchArgs.ConeSegmentCount)) +
        (12 * BatchArgs.OBBCount);

    Graphics->DeviceContext->DrawInstanced(VertexCountPerInstance, InstanceCount, 0, 0);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void FLineRenderPass::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;
    
    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

void FLineRenderPass::ProcessLineRendering(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    UpdateShader();
    PrepareLineShader();

    // 상수 버퍼 업데이트: Identity 모델, 기본 색상 등
    UpdateObjectConstant(FMatrix::Identity, FVector4(0, 0, 0, 0), false);

    FLinePrimitiveBatchArgs BatchArgs;
    FEngineLoop::PrimitiveDrawBatch.PrepareBatch(BatchArgs);
    DrawLineBatch(BatchArgs);
    FEngineLoop::PrimitiveDrawBatch.RemoveArr();
}

void FLineRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    constexpr EResourceType ResourceType = EResourceType::ERT_Editor;

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);

    ProcessLineRendering(Viewport);

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}
