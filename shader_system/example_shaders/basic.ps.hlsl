// basic.ps.hlsl – Simple pixel shader
// Samples a diffuse texture and applies a directional light.

Texture2D    DiffuseMap : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer LightData : register(b1)
{
    float3 LightDir;
    float  Padding;
    float3 LightColor;
    float  Intensity;
};

struct PSInput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
};

float4 PSMain(PSInput IN) : SV_Target
{
    float4 diffuse  = DiffuseMap.Sample(LinearSampler, IN.TexCoord);
    float  NdotL    = saturate(dot(normalize(IN.Normal), -normalize(LightDir)));
    float3 lighting = LightColor * Intensity * NdotL;
    return float4(diffuse.rgb * lighting, diffuse.a);
}
