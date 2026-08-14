cbuffer SceneConstants : register(b0)
{
    row_major float4x4 viewProjection;
};

Texture2D spriteTexture : register(t0);
SamplerState spriteSampler : register(s0);

struct VertexInput
{
    float3 position : POSITION;
    float2 textureCoordinate : TEXCOORD;

    float4 worldRow0 : INSTANCE_WORLD0;
    float4 worldRow1 : INSTANCE_WORLD1;
    float4 worldRow2 : INSTANCE_WORLD2;
    float4 worldRow3 : INSTANCE_WORLD3;

    float4 tintColor : INSTANCE_COLOR;
    float4 uvRectangle : INSTANCE_UV;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 textureCoordinate : TEXCOORD;
    float4 tintColor : COLOR;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    row_major float4x4 world = float4x4(
        input.worldRow0,
        input.worldRow1,
        input.worldRow2,
        input.worldRow3);

    const float4 worldPosition =
        mul(
            float4(input.position, 1.0F),
            world);

    output.position =
        mul(
            worldPosition,
            viewProjection);

    output.textureCoordinate =
        input.uvRectangle.xy +
        input.textureCoordinate *
        input.uvRectangle.zw;

    output.tintColor =
        input.tintColor;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    const float4 textureColor =
        spriteTexture.Sample(
            spriteSampler,
            input.textureCoordinate);

    clip(textureColor.a - 0.05F);

    return textureColor *
        input.tintColor;
}