// pbr.vs.hlsl – Advanced Physically Based Rendering vertex shader
// Supports variants: SKINNED, INSTANCED, USE_NORMAL_MAP

cbuffer PerFrame : register(b0)
{
    matrix View;
    matrix Projection;
    float3 CameraPosition;
    float  Time;
};

#ifdef INSTANCED
// Per-instance world matrix supplied via vertex buffer
struct InstanceData
{
    matrix World : WORLD;
};
#else
cbuffer PerObject : register(b1)
{
    matrix World;
    matrix WorldInvTranspose;
};
#endif

#ifdef SKINNED
#ifndef MAX_BONES
#define MAX_BONES 128
#endif
cbuffer SkinData : register(b2)
{
    matrix BoneMatrices[MAX_BONES];
};
#endif

struct VSInput
{
    float3 Position  : POSITION;
    float3 Normal    : NORMAL;
    float2 TexCoord  : TEXCOORD0;
    float4 Tangent   : TANGENT;

#ifdef SKINNED
    uint4  BoneIds     : BLENDINDICES;
    float4 BoneWeights : BLENDWEIGHT;
#endif

#ifdef INSTANCED
    matrix InstanceWorld : WORLD;
#endif
};

struct VSOutput
{
    float4 ClipPosition : SV_Position;
    float3 WorldPos     : TEXCOORD0;
    float3 WorldNormal  : TEXCOORD1;
    float2 TexCoord     : TEXCOORD2;
#ifdef USE_NORMAL_MAP
    float3 WorldTangent  : TEXCOORD3;
    float3 WorldBitangent : TEXCOORD4;
#endif
};

VSOutput VSMain(VSInput IN
#ifdef INSTANCED
    , uint instanceID : SV_InstanceID
#endif
)
{
    VSOutput OUT;

    // Determine world matrix
#ifdef INSTANCED
    matrix W = IN.InstanceWorld;
#else
    matrix W = World;
#endif

    float4 localPos = float4(IN.Position, 1.0f);

#ifdef SKINNED
    // Linear blend skinning
    float4 skinnedPos = float4(0, 0, 0, 0);
    float3 skinnedNorm = float3(0, 0, 0);
    [unroll(4)]
    for (int i = 0; i < 4; ++i)
    {
        uint   boneIdx = IN.BoneIds[i];
        float  weight  = IN.BoneWeights[i];
        skinnedPos  += weight * mul(localPos, BoneMatrices[boneIdx]);
        skinnedNorm += weight * (float3)mul(float4(IN.Normal, 0.0f), BoneMatrices[boneIdx]);
    }
    localPos    = skinnedPos;
    IN.Normal   = normalize(skinnedNorm);
#endif

    float4 worldPos = mul(localPos, W);
    OUT.WorldPos    = worldPos.xyz;
    OUT.ClipPosition = mul(mul(worldPos, View), Projection);
    OUT.TexCoord    = IN.TexCoord;
    OUT.WorldNormal = normalize(mul(float4(IN.Normal, 0.0f), W).xyz);

#ifdef USE_NORMAL_MAP
    OUT.WorldTangent   = normalize(mul(float4(IN.Tangent.xyz, 0.0f), W).xyz);
    OUT.WorldBitangent = cross(OUT.WorldNormal, OUT.WorldTangent) * IN.Tangent.w;
#endif

    return OUT;
}
