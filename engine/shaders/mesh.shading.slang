struct SceneData
{
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;
    float4 ambient_color;
    float4 sunlight_direction;
    float4 sunlight_color;
}

struct GLTFMaterialData{   
	float4 colorFactors;
	float4 metal_rough_factors;
}

[[vk::binding(0, 0)]]
ConstantBuffer<SceneData> scene_data;

[[vk::binding(0, 1)]]
ConstantBuffer<GLTFMaterialData> gltf_material_data;

[[vk::binding(1, 1)]]
Sampler2D color_texture;

[[vk::binding(2, 1)]]
Sampler2D metal_rough_texture;

struct Vertex
{
	float3 position;
	float uv_x;
	float3 normal;
	float uv_y;
	float4 color;
};

struct VertexOutput 
{
	float4 position : SV_Position;
    float3 normal;
    float3 color;
	float2 uv;
};

[shader("vertex")]
VertexOutput vert_main(uint vertexID : SV_VertexID, [[push_constant]] uniform float4x4 render_matrix, [[push_constant]] uniform Vertex* vertex_buffer)
{
    Vertex v = vertex_buffer[vertexID];

    float4 position = float4(v.position, 1.0f);
    VertexOutput out;
    out.position = mul(scene_data.view_proj, mul(render_matrix, position));
    out.normal = mul(render_matrix, float4(v.normal, 0.0f)).xyz;
    out.color = v.color.rgb * gltf_material_data.colorFactors.rgb;
    out.uv = float2(v.uv_x, v.uv_y);
    return out;
}

[shader("fragment")]
float4 frag_main(VertexOutput in) : SV_Target
{
    float light_value = max(dot(in.normal, scene_data.sunlight_direction.xyz), 0.1f);

    float3 color = in.color * color_texture.Sample(in.uv).rgb;
    float3 ambient = color *scene_data.ambient_color.rgb;

    return float4(color * light_value * scene_data.sunlight_color.w + ambient, 1.f );
}