#pragma once
#include "IRenderPass.h"
#include "Animation/AnimNotifyState.h"

class FDoFRenderPass: public IRenderPass
{
public:
    FDoFRenderPass();
    ~FDoFRenderPass();
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    virtual void PrepareRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void ClearRenderArr() override;

    void PrepareRenderState();
    void CreateShader();
    void UpdateHShader();
    void UpdateVShader();
    void ReleaseShader();
    
    void CreateSampler();

private:
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FDXDShaderManager* ShaderManager;

    ID3D11SamplerState* Sampler;

    ID3D11VertexShader* VertexShader;
    ID3D11PixelShader* PixelHShader;
    ID3D11PixelShader* PixelVShader;
};
