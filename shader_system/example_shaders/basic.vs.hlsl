// basic.vs.hlsl – Simple vertex shader
// Transforms a position and passes a UV coordinate to the pixel stage.

cbuffer PerObject : register(b0)
{
    matrix WorldViewProjection;
};

struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
};

VSOutput VSMain(VSInput IN)
{
    VSOutput OUT;
    OUT.Position = mul(float4(IN.Position, 1.0f), WorldViewProjection);
    OUT.TexCoord = IN.TexCoord;
    OUT.Normal   = IN.Normal;
    return OUT;
}
