cbuffer SceneConstants : register(b0)
{
    row_major float4x4 viewProjection;
};

Texture2D groundTexture : register(t0);
SamplerState groundSampler : register(s0);

struct VertexInput
{
    float3 position : POSITION;
    float2 textureCoordinate : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 textureCoordinate : TEXCOORD;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    output.position = mul(
        float4(input.position, 1.0F),
        viewProjection);

    output.textureCoordinate =
        input.textureCoordinate;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    return groundTexture.Sample(
        groundSampler,
        input.textureCoordinate);
}