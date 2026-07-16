// basic.vs.hlsl – Simple vertex shader
// Entry point: VSMain

cbuffer PerObject : register(b0) {
    float4x4 gWorld;
    float4x4 gViewProj;
};

struct VertexIn {
    float3 posL   : POSITION;
    float3 normalL: NORMAL;
    float2 texC   : TEXCOORD;
};

struct VertexOut {
    float4 posH   : SV_POSITION;
    float3 posW   : POSITION;
    float3 normalW: NORMAL;
    float2 texC   : TEXCOORD;
};

VertexOut VSMain(VertexIn vin) {
    VertexOut vout;
    float4 posW = mul(float4(vin.posL, 1.0f), gWorld);
    vout.posH    = mul(posW, gViewProj);
    vout.posW    = posW.xyz;
    vout.normalW = mul(float4(vin.normalL, 0.0f), gWorld).xyz;
    vout.texC    = vin.texC;
    return vout;
}
