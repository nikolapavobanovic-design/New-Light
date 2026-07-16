// basic.ps.hlsl – Simple pixel shader
// Entry point: PSMain

Texture2D    gDiffuseMap : register(t0);
SamplerState gSampler    : register(s0);

cbuffer PerFrame : register(b1) {
    float3 gLightDir;
    float  gPad;
    float3 gLightColor;
    float  gPad2;
    float3 gAmbient;
    float  gPad3;
};

struct PixelIn {
    float4 posH   : SV_POSITION;
    float3 posW   : POSITION;
    float3 normalW: NORMAL;
    float2 texC   : TEXCOORD;
};

float4 PSMain(PixelIn pin) : SV_TARGET {
    float3 normal   = normalize(pin.normalW);
    float  NdotL    = saturate(dot(normal, -normalize(gLightDir)));
    float4 diffuse  = gDiffuseMap.Sample(gSampler, pin.texC);

    float3 ambient  = gAmbient * diffuse.rgb;
    float3 lighting = ambient + NdotL * gLightColor * diffuse.rgb;
    return float4(lighting, diffuse.a);
}
