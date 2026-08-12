cbuffer ObjectConstants : register(b0)
{
    row_major float4x4 worldViewProjection;
    float4 tintColor;
};

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    output.position = mul(
        float4(input.position, 1.0F),
        worldViewProjection);

    output.color = tintColor;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    return input.color;
}