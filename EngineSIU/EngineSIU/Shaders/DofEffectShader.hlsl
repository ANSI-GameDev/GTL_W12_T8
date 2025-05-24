Texture2D depthTexture : register(t99);
Texture2D inputTexture : register(t100);
SamplerState samplerState : register(s0);

cbuffer DoFBlurConstants : register(b0)
{
    float2 texelSize;
    float focusDistance;
    float blurScale;
};

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

PS_Input mainVS(uint VertexID : SV_VertexID)
{
    PS_Input Output;

    float2 QuadPositions[6] =
    {
        float2(-1, 1), // Top Left
        float2(1, 1), // Top Right
        float2(-1, -1), // Bottom Left
        float2(1, 1), // Top Right
        float2(1, -1), // Bottom Right
        float2(-1, -1) // Bottom Left
    };

    float2 UVs[6] =
    {
        float2(0, 0), float2(1, 0), float2(0, 1),
        float2(1, 0), float2(1, 1), float2(0, 1)
    };

    Output.Position = float4(QuadPositions[VertexID], 0, 1);
    Output.UV = UVs[VertexID];

    return Output;
}

float4 PS_BlurH(PS_Input Input) : SV_Target
{
    float GaussianWeights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
    float4 color = inputTexture.Sample(samplerState, Input.UV) * GaussianWeights[0];
    float depth = depthTexture.Sample(samplerState, Input.UV).r;
    float distance = abs(depth - focusDistance);
    float blurStrength = saturate(distance * blurScale);
    
    for (int i = 1; i < 5; ++i)
    {
        float offset = texelSize.x * i * blurStrength;
        color += inputTexture.Sample(samplerState, Input.UV + float2(offset, 0)) * GaussianWeights[i];
        color += inputTexture.Sample(samplerState, Input.UV - float2(offset, 0)) * GaussianWeights[i];
    }
    
    return color;
}

float4 PS_BlurV(PS_Input Input) : SV_Target
{
    float GaussianWeights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
    float4 color = inputTexture.Sample(samplerState, Input.UV) * GaussianWeights[0];
    float depth = depthTexture.Sample(samplerState, Input.UV).r;
    float distance = abs(depth - focusDistance);
    float blurStrength = saturate(distance * blurScale);

    for (int i = 1; i < 5; ++i)
    {
        float offset = texelSize.y * i * blurStrength;
        color += inputTexture.Sample(samplerState, Input.UV + float2(0, offset)) * GaussianWeights[i];
        color += inputTexture.Sample(samplerState, Input.UV - float2(0, offset)) * GaussianWeights[i];
    }
    
    return color;
}
