struct VertexOutput 
{
	float4 position : SV_Position;
    float3 color;
	float2 uv;
};

struct Vertex
{
	float3 position;
	float uv_x;
	float3 normal;
	float uv_y;
	float4 color;
};

[shader("vertex")]
VertexOutput vert_main(uint vertexID : SV_VertexID, [[push_constant]] uniform float4x4 world_matrix, [[push_constant]] uniform Vertex* vertex_buffer)
{
	Vertex v = vertex_buffer[vertexID];
	
    VertexOutput output;
	output.position = mul(world_matrix, float4(v.position, 1));
    output.color = v.color.rgb;
	output.uv = float2(v.uv_x, v.uv_y);
    return output;
}

Sampler2D texture;

[shader("fragment")]
float4 frag_main(VertexOutput in_vert) : SV_Target
{
    return texture.Sample(in_vert.uv);
}