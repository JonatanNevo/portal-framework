struct VertexInput
{
    float3 in_position;
    float3 in_color;
    float2 in_texcoord;
};

struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};
ConstantBuffer<UniformBuffer> ubo;

struct VertexOutput 
{
    float3 color;
    float2 frag_texcoord;
    float4 position : SV_Position;
};

[shader("vertex")]
VertexOutput vert_main(VertexInput input)
{
    VertexOutput output;
    output.position = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.in_position, 1.0))));
    output.color = input.in_color;
    output.frag_texcoord = input.in_texcoord;
    return output;
}

Sampler2D texture;

[shader("fragment")]
float4 frag_main(VertexOutput in_vert) : SV_Target
{
    return texture.Sample(in_vert.frag_texcoord);
}