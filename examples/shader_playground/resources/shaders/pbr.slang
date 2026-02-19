import portal_common;
import portal_lights;
import portal_scene;
import portal_pbr;

const static float3 f_dielectric = float3(0.04);

public extern const static bool has_normal_texture;
public extern const static bool has_metallic_texture;
public extern const static bool has_roughness_texture;

struct MaterialData
{
    float3 albedo;
    float metallic;
    float roughness;
    float emission;
    float env_map_rotation;

    Sampler2D color_texture;
    Conditional<Sampler2D, has_normal_texture> normal_texture;
    Conditional<Sampler2D, has_metallic_texture> metallic_texture;
    Conditional<Sampler2D, has_roughness_texture> roughness_texture;
}

ParameterBlock<SceneLights> scene_lights;
ParameterBlock<SceneData> scene_data;

ParameterBlock<MaterialData> material_data;


struct Vertex
{
	float3 position;
	float uv_x;
	float3 normal;
	float uv_y;
    float4 tangent; // xyz = tangent, w = handedness
};

struct VertexOutput
{
    uint vertexID;
    float4 position : SV_Position;

    float3 world_position;
    float3 normal;
    float4 tangent;
    float2 uv;

    float3x3 world_transform;
};

[shader("vertex")]
VertexOutput vert_main(uint vertexID : SV_VertexID, uniform float4x4 render_matrix,  uniform Vertex* vertex_buffer)
{
    Vertex v = vertex_buffer[vertexID];
    VertexOutput output;

    float4 world_position = mul(render_matrix, float4(v.position, 1.0f));

    output.world_position = world_position.xyz;
    output.normal = mul(render_matrix, float4(v.normal, 0.0f)).xyz;
    output.uv = float2(v.uv_x, v.uv_y); // TODO: vec2(a_TexCoord.x, 1.0 - a_TexCoord.y); ?
    output.tangent = float4(mul(render_matrix, float4(v.tangent.xyz, 0.0f)).xyz, v.tangent.w);
    output.world_transform = float3x3(render_matrix);

    output.position = mul(scene_data.camera.view_proj, world_position);
    output.vertexID = vertexID;

    return output;
}

struct FragOutput
{
    float4 color : SV_Target0;
    float4 view_normal_luminance : SV_Target1;
    float4 metalness_roughness : SV_Target2;
};

float3 ibl(float3 f0, float3 lr, PBRParameters params, SceneData scene_data)
{
    float3 irradiance = scene_data.environment_irradiance_texture.Sample(params.normal).rgb;
    float3 f = fresnel_schlick_roughness(f0, params.ndotv, params.roughness);
    float3 kd = (1.f - f) * (1.f - params.metallic);
    float3 diffuse_ibl = params.albedo * irradiance;

    uint width, height, env_radiance_tex_levels;
    scene_data.environment_radiance_texture.GetDimensions(0, width, height, env_radiance_tex_levels);
    float3 specular_irradiance = scene_data.environment_radiance_texture.SampleLevel(
                                                                            rotate_vector_about_y(material_data.env_map_rotation, lr),
                                                                            params.roughness * float(env_radiance_tex_levels)
                                                                        ) .rgb;
    
    float2 specular_brdf = scene_data.brdf_lut_texture.Sample(float2(params.ndotv, params.roughness)).rg;
    float3 spebular_ibl = specular_irradiance * (f0 * specular_brdf.x + specular_brdf.y);
    return kd * diffuse_ibl + spebular_ibl;
}

/////////////////////////////////////////////

float3 get_gradient(float value)
{
    float3 zero = float3(0.0, 0.0, 0.0);
    float3 white = float3(0.0, 0.1, 0.9);
    float3 red = float3(0.2, 0.9, 0.4);
    float3 blue = float3(0.8, 0.8, 0.3);
    float3 green = float3(0.9, 0.2, 0.3);

    float step0 = 0.0f;
    float step1 = 2.0f;
    float step2 = 4.0f;
    float step3 = 8.0f;
    float step4 = 16.0f;

    float3 color = lerp(zero, white, smoothstep(step0, step1, value));
    color = lerp(color, white, smoothstep(step1, step2, value));
    color = lerp(color, red, smoothstep(step1, step2, value));
    color = lerp(color, blue, smoothstep(step2, step3, value));
    color = lerp(color, green, smoothstep(step3, step4, value));

    return color;
}

[shader("fragment")]
FragOutput frag_main(VertexOutput in)
{
    FragOutput output;

    float2 uv = in.uv;
    float3 normalized_normal_ws = in.normal;
    float renorm_factor = 1.f / length(normalized_normal_ws);

    float3x3 world_to_tangent;
    float3 bitanget = cross(normalized_normal_ws, in.tangent.xyz) * in.tangent.w;
    world_to_tangent[0] = in.tangent.xyz * renorm_factor;
    world_to_tangent[1] = bitanget * renorm_factor;
    world_to_tangent[2] = normalized_normal_ws * renorm_factor;

    PBRParameters params;
    float4 albedo_color = material_data.color_texture.Sample(uv);
    params.albedo = albedo_color.rgb * to_linear(float4(material_data.albedo, 1.f)).rgb;
    float alpha = albedo_color.a;

    // note: Metalness and roughness could be in the same texture.
    //       Per GLTF spec, we read metalness from the B channel and roughness from the G channel
    //       This will still work if metalness and roughness are independent greyscale textures,
    //       but it will not work if metalness and roughness are independent textures containing only R channel.
    if (let texture = material_data.metallic_texture.get())
    {
        params.metallic = texture.Sample(uv).b * material_data.metallic;
    }
    else
    {
        params.metallic = material_data.metallic;
    }
    
    if (let texture = material_data.roughness_texture.get())
    {
        params.roughness = texture.Sample(uv).g * material_data.roughness;
    }
    else
    {
        params.roughness = material_data.roughness;
    }

    output.metalness_roughness = float4(params.metallic, params.roughness, 0.f, 1.f);
    params.roughness = max(params.roughness, 0.05f); // Minimum roughness of 0.05 to keep specular highlight

    // Normals
    if (let texture = material_data.normal_texture.get())
    {
        params.normal = normalize(texture.Sample(uv).xyz * 2.f - 1.f);
        params.normal = mul(params.normal, world_to_tangent);
    }
    else
    {
        params.normal = normalize(in.normal);
    }

    output.view_normal_luminance.xyz = mul(float3x3(scene_data.camera.view), params.normal);

    float3 camera_position = float3(scene_data.camera.inverse_view[0][3], scene_data.camera.inverse_view[1][3], scene_data.camera.inverse_view[2][3]);
    params.view = normalize(camera_position - in.world_position);
    params.ndotv = max(dot(params.normal, params.view), 0.f);

    // Specualr reflection vector;
    float3 lr = 2.f * params.ndotv * params.normal - params.view;

    // Fresnel reflectance
    float3 f0 = lerp(f_dielectric, params.albedo, params.metallic);

    // TODO implement shadows

    // Direct lighting
    float3 light_contribution = calculate_directional_lights(f0, params, scene_lights.directional_light);
    // TODO: add point lights
    // TODO: add spot lights
    light_contribution += params.albedo * material_data.emission;

    // Indirect lighting
    float3 ibl_contribution = ibl(f0, lr, params, scene_data) * scene_lights.directional_light.environment_map_intensity;

    // final color
    output.color = float4(ibl_contribution + light_contribution, 1.f);
    return output;
}