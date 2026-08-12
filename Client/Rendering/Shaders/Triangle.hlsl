cbuffer SceneConstants : register(b0)
{
    row_major float4x4 viewProjection;
};

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;

    float4 worldRow0 : INSTANCE_WORLD0;
    float4 worldRow1 : INSTANCE_WORLD1;
    float4 worldRow2 : INSTANCE_WORLD2;
    float4 worldRow3 : INSTANCE_WORLD3;

    float4 tintColor : INSTANCE_COLOR;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    row_major float4x4 world = float4x4(
        input.worldRow0,
        input.worldRow1,
        input.worldRow2,
        input.worldRow3);

    float4 worldPosition = mul(
        float4(input.position, 1.0F),
        world);

    output.position = mul(
        worldPosition,
        viewProjection);

    output.color = input.tintColor;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    return input.color;
}