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

struct GlobalData
{
    SceneData scene;
};

struct MaterialData
{
    GLTFMaterialData gltf_material_data;
    Sampler2D color_texture;
    Sampler2D metal_rough_texture;
};

ParameterBlock<GlobalData> global_data;
ParameterBlock<MaterialData> material_data;

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
    out.position = mul(global_data.scene.view_proj, mul(render_matrix, position));
    out.normal = mul(render_matrix, float4(v.normal, 0.0f)).xyz;
    out.color = v.color.rgb * material_data.gltf_material_data.colorFactors.rgb;
    out.uv = float2(v.uv_x, v.uv_y);
    return out;
}

[shader("fragment")]
float4 frag_main(VertexOutput in) : SV_Target
{
    float light_value = max(dot(in.normal, global_data.scene.sunlight_direction.xyz), 0.1f);

    float3 color = in.color * material_data.color_texture.Sample(in.uv).rgb;
    float3 ambient = color * global_data.scene.ambient_color.rgb;

    return float4(color * light_value * global_data.scene.sunlight_color.w + ambient, 1.f );
}