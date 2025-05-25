Texture2D depthTexture : register(t99);
Texture2D inputTexture : register(t100);
SamplerState samplerState : register(s0);

cbuffer DoFBlurConstants : register(b0)
{
    float2 texelSize;
    float focusDistance;
    float focusDepth;
    float blurScale;
    float nearClip;
    float farClip;
    float padding;
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
    float4 texColor = inputTexture.Sample(samplerState, Input.UV);
    float4 color = texColor;
    float depth = depthTexture.Sample(samplerState, Input.UV).r;
    float linearDepth = nearClip * farClip / (farClip - depth * (farClip - nearClip));
    float maxCoC = blurScale * abs(linearDepth - focusDistance) / linearDepth;

    for (int y = -7; y < 7; ++y)
    {
        for (int x = -7; x < 7; ++x)
        {
            float2 offset = float2(x, y) * texelSize;
            float sampleDepth = depthTexture.Sample(samplerState, Input.UV + offset);
            float linearSampleDepth = nearClip * farClip / (farClip - sampleDepth * (farClip - nearClip));
            float sampleCoC = abs(linearSampleDepth - focusDistance) / linearSampleDepth;
            if (linearSampleDepth < linearDepth)
                maxCoC = max(maxCoC, sampleCoC);
        }
    }
    
    float blurWidth = texelSize.x * maxCoC;
    float sampleWeights = 1.0f;

    [loop]
    for (float offset = texelSize.x; offset < blurWidth; offset += texelSize.x)
    {
        {
            float sampleDepth = depthTexture.Sample(samplerState, Input.UV + float2(offset, 0));
            float linearSampleDepth = nearClip * farClip / (farClip - sampleDepth * (farClip - nearClip));
            float sampleCoC = abs(linearSampleDepth - focusDistance) / linearSampleDepth;
            float weight = (blurWidth - offset) / blurWidth * (sampleCoC / maxCoC);
            color += inputTexture.Sample(samplerState, Input.UV + float2(offset, 0)) * weight;
            sampleWeights += weight;
        }
        {
            float sampleDepth = depthTexture.Sample(samplerState, Input.UV - float2(offset, 0));
            float linearSampleDepth = nearClip * farClip / (farClip - sampleDepth * (farClip - nearClip));
            float sampleCoC = abs(linearSampleDepth - focusDistance) / linearSampleDepth;
            float weight = (blurWidth - offset) / blurWidth * (sampleCoC / maxCoC);
            color += inputTexture.Sample(samplerState, Input.UV - float2(offset, 0)) * weight;
            sampleWeights += weight;
        }
    }
    color = color / sampleWeights;
    return color;
}

float4 PS_BlurV(PS_Input Input) : SV_Target
{
    float4 texColor = inputTexture.Sample(samplerState, Input.UV);
    float4 color = texColor;
    float depth = depthTexture.Sample(samplerState, Input.UV).r;
    float linearDepth = nearClip * farClip / (farClip - depth * (farClip - nearClip));
    float maxCoC = blurScale * abs(linearDepth - focusDistance) / linearDepth;
    
    for (int y = -7; y < 7; ++y)
    {
        for (int x = -7; x < 7; ++x)
        {
            float2 offset = float2(x, y) * texelSize;
            float sampleDepth = depthTexture.Sample(samplerState, Input.UV + offset);
            float linearSampleDepth = nearClip * farClip / (farClip - sampleDepth * (farClip - nearClip));
            float sampleCoC = abs(linearSampleDepth - focusDistance) / linearSampleDepth;
            if (linearSampleDepth < linearDepth)
                maxCoC = max(maxCoC, sampleCoC);
        }
    }
    
    float blurWidth = texelSize.y * maxCoC;
    float SampleWeights = 1.0f;

    [loop]
    for (float offset = texelSize.y; offset < blurWidth; offset += texelSize.y)
    {
        {
            float sampleDepth = depthTexture.Sample(samplerState, Input.UV + float2(0, offset));
            float linearSampleDepth = nearClip * farClip / (farClip - sampleDepth * (farClip - nearClip));
            float sampleCoC = abs(linearSampleDepth - focusDistance) / linearSampleDepth;
            float weight = (blurWidth - offset) / blurWidth * (sampleCoC / maxCoC);
            color += inputTexture.Sample(samplerState, Input.UV + float2(0, offset)) * weight;
            SampleWeights += weight;
        }
        {
            float sampleDepth = depthTexture.Sample(samplerState, Input.UV - float2(0, offset));
            float linearSampleDepth = nearClip * farClip / (farClip - sampleDepth * (farClip - nearClip));
            float sampleCoC = abs(linearSampleDepth - focusDistance) / linearSampleDepth;
            float weight = (blurWidth - offset) / blurWidth * (sampleCoC / maxCoC);
            color += inputTexture.Sample(samplerState, Input.UV - float2(0, offset)) * weight;
            SampleWeights += weight;
        }       
    }
    color = color / SampleWeights;
    return color;
}
