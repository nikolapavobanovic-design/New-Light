// pbr.vs.hlsl – Physically Based Rendering vertex shader
// Supports compile-time variants via preprocessor defines:
//   SKINNED    – skeletal animation (up to 4 bone influences)
//   INSTANCED  – GPU instancing via per-instance world matrix

cbuffer PerObject : register(b0) {
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4x4 gViewProj;
};

#ifdef INSTANCED
struct InstanceData {
    float4x4 world;
};
StructuredBuffer<InstanceData> gInstances : register(t5);
#endif

#ifdef SKINNED
cbuffer SkinningData : register(b2) {
    float4x4 gBoneTransforms[256];
};
#endif

struct VertexIn {
    float3 posL    : POSITION;
    float3 normalL : NORMAL;
    float3 tangentL: TANGENT;
    float2 texC    : TEXCOORD;
#ifdef SKINNED
    float4 weights : BLENDWEIGHT;
    uint4  indices : BLENDINDICES;
#endif
#ifdef INSTANCED
    uint   instanceID : SV_InstanceID;
#endif
};

struct VertexOut {
    float4 posH    : SV_POSITION;
    float3 posW    : POSITION;
    float3 normalW : NORMAL;
    float3 tangentW: TANGENT;
    float2 texC    : TEXCOORD;
};

VertexOut VSMain(VertexIn vin) {
    VertexOut vout;

    float3 posL    = vin.posL;
    float3 normalL = vin.normalL;

#ifdef SKINNED
    // Blend bone transforms.
    float4 blendedPos    = float4(0, 0, 0, 0);
    float4 blendedNormal = float4(0, 0, 0, 0);
    [unroll]
    for (int i = 0; i < 4; ++i) {
        blendedPos    += vin.weights[i] *
            mul(float4(posL, 1.0f),    gBoneTransforms[vin.indices[i]]);
        blendedNormal += vin.weights[i] *
            mul(float4(normalL, 0.0f), gBoneTransforms[vin.indices[i]]);
    }
    posL    = blendedPos.xyz;
    normalL = normalize(blendedNormal.xyz);
#endif

#ifdef INSTANCED
    float4x4 world = gInstances[vin.instanceID].world;
#else
    float4x4 world = gWorld;
#endif

    float4 posW  = mul(float4(posL, 1.0f), world);
    vout.posH    = mul(posW, gViewProj);
    vout.posW    = posW.xyz;
    vout.normalW = normalize(mul(float4(normalL, 0.0f), world).xyz);
    vout.tangentW= normalize(mul(float4(vin.tangentL, 0.0f), world).xyz);
    vout.texC    = vin.texC;
    return vout;
}
