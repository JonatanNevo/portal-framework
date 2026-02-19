import portal_common;

const static float normal_strength = 1.0f;

public extern const static bool has_normal_texture;
public extern const static bool has_tangent_texture;
public extern const static bool has_metallic_roughness_texture;

struct SceneData
{
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;
    float4x4 inverse_view;
    float4x4 inverse_proj;
    float4x4 inverse_view_proj;

    float4 ambient_color;
    float4 sunlight_direction;
    float4 sunlight_color;
}

struct MaterialData {
    // Diffuse
    float4 surface_color;
    float roughness;
    float subsurface;
    float sheen;
    float sheen_tint;

    // Specular
    float anistropy;
    float specular_strength;
    float metallic;
    float specular_tint;

    // Clearcoat
    float clearcoat;
    float clearcoat_gloss;

    // Textures;
    Sampler2D color_texture;
    Conditional<Sampler2D, has_normal_texture> normal_texture;
    Conditional<Sampler2D, has_tangent_texture> tangent_texture;
    Conditional<Sampler2D, has_metallic_roughness_texture> metallic_roughness_texture;
}

ParameterBlock<SceneData> scene_data;
ParameterBlock<MaterialData> material_data;

struct Vertex
{
	float3 position;
	float uv_x;
	float3 normal;
	float uv_y;
    float4 tangent; // xyz = tangent, w = handedness
	float4 color;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv;
    float3 normal;
    float4 tangent;
    float3 world_position;
    float3 object_position;
    float3 color;
};

[shader("vertex")]
VertexOutput vert_main(uint vertexID : SV_VertexID, [[push_constant]] uniform float4x4 render_matrix, [[push_constant]] uniform Vertex* vertex_buffer)
{
    Vertex v = vertex_buffer[vertexID];
    VertexOutput out;

    float4 position = float4(v.position, 1.0f);

    out.world_position = mul(render_matrix, position).xyz;
    out.position = mul(scene_data.view_proj, float4(out.world_position, 1.f));
    out.object_position = v.position;

    out.normal = mul(render_matrix, float4(v.normal, 0.0f)).xyz;
    out.tangent = float4(mul(render_matrix, float4(v.tangent.xyz, 0.0f)).xyz, v.tangent.w);
    out.uv = float2(v.uv_x, v.uv_y);
    out.color = v.color.rgb * material_data.surface_color.rgb;

    return out;
}

struct BRDFInput
{
    float3 L; // Direction to light source
    float3 V; // Direction to camera/viewer
    float3 N; // World space normal (unpacked from normal map)
    float3 X; // World space tangent (unpacked from tangent map)
    float3 Y; // World space bitangent
    float roughness; // From uniform or texture map
    float metallic;
    float3 base_color;
}

struct BRDFResults 
{
    float3 diffuse;
    float3 specular;
    float3 clearcoat;
};

float luminance(float3 color) 
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float fresnel_shlick(float x)
{
    x = saturate(1.f - x);
    float x2 = x * x;
    return x2 * x2 * x; // While this is equivalent to pow(1 - x, 5) it is two less mult instructions
}

// Anisotropic Generalized Trowbridge Reitz with gamma == 2. This is equal to the popular GGX distribution.
float anistropic_gtr2(float ndoth, float hdotx, float hdoty, float ax, float ay)
{
    return rcp(PI * ax * ay * sqr(sqr(hdotx / ax) + sqr(hdoty / ay) + sqr(ndoth)));
}

// Anisotropic Geometric Attenuation Function for GGX.
float anistropic_smith_gxx(float ndots, float sdotx, float sdoty, float ax, float ay)
{
    return rcp(ndots + sqrt(sqr(sdotx * ax) + sqr(sdoty * ay) + sqr(ndots)));
}

// Isotropic Generalized Trowbridge Reitz with gamma == 1
float gtr1(float ndoth, float a)
{
    float a2 = a * a;
    float t = 1.0f + (a2 - 1.0f) * ndoth * ndoth;
    return (a2 - 1.0f) / (PI * log(a2) * t);
}

// Isotropic Geometric Attenuation Function for GGX. This is technically different from what Disney uses, but it's basically the same.
float smith_gxx(float alpha_squared, float ndotl, float ndotv)
{
    float a = ndotv * sqrt(alpha_squared + ndotl * (ndotl - alpha_squared * ndotl));
    float b = ndotl * sqrt(alpha_squared + ndotv * (ndotv - alpha_squared * ndotv));

    return 0.5f / (a + b);
}

BRDFResults disney_brdf(BRDFInput input)
{
    BRDFResults results;
    results.diffuse = float3(0.0f);
    results.specular = float3(0.0f);
    results.clearcoat = float3(0.0f);

    // Microfacet normal of perfect reflection
    float3 H = normalize(input.L + input.V); 

    float ndotl = saturate(dot(input.N, input.L));
    float ndotv = saturate(dot(input.N, input.V));
    float ndoth = saturate(dot(input.N, H));
    float ldoth = saturate(dot(input.L, H));

    float color_luminance = luminance(input.base_color);

    float3 color_tint = color_luminance > 0.f ? input.base_color / color_luminance : float3(1.f, 1.f, 1.f);
    float3 color_specular0 = lerp(material_data.specular_strength * 0.08f * lerp(1.f, color_tint, material_data.specular_tint), input.base_color * (1.f + material_data.specular_strength), input.metallic);
    float3 color_sheen = lerp(1.f, color_tint, material_data.sheen_tint);

    // Diffuse
    float fl = fresnel_shlick(ndotl);
    float fv = fresnel_shlick(ndotv);

    float Fss90 = ldoth * ldoth * input.roughness;
    float Fd90 = 0.5f + 2.0f * Fss90;

    float Fd = lerp(1.0f, Fd90, fl) * lerp(1.0f, Fd90, fv);

    // Subsurface Diffuse (Hanrahan-Krueger brdf approximation)
    float Fss = lerp(1.f, Fss90, fl) * lerp(1.f, Fss90, fv);
    float ss = 1.25f * (Fss * (rcp(ndotl + ndotv) - 0.5f) + 0.5f);

    // Specular
    float alpha_squared = sqr(input.roughness);

    // Anisotropic Microfacet Normal Distribution (Normalized Anisotropic GTR gamma == 2)
    float aspect_ratio = sqrt(1.f - material_data.anistropy * 0.9f);
    float alpha_x = max(0.001f, alpha_squared / aspect_ratio);
    float alpha_y = max(0.001f, alpha_squared * aspect_ratio);
    float Ds = anistropic_gtr2(ndoth, dot(H, input.X), dot(H, input.Y), alpha_x, alpha_y);

    // Geometric Attenuation
    float g_alpha_squared = sqr(0.5f + input.roughness * 0.5f);
    float g_alpha_x = max(0.001f, g_alpha_squared / aspect_ratio);
    float g_alpha_y = max(0.001f, g_alpha_squared * aspect_ratio);
    float G = anistropic_smith_gxx(ndotl, dot(input.L, input.X), dot(input.L, input.Y), g_alpha_x, g_alpha_y);
    G *= anistropic_smith_gxx(ndotv, dot(input.V, input.X), dot(input.V, input.Y), g_alpha_x, g_alpha_y); // specular brdf denominator (4 * ndotl * ndotv) is baked into output here

    // Fresnel Reflectance
    float FH = fresnel_shlick(ldoth);
    float3 F = lerp(color_specular0, 1.f, FH);

    // Sheen
    float3 Fsheen = lerp(color_specular0, 1.f, fresnel_shlick(ndotv)) * material_data.sheen * color_sheen;

    // Clearcoat (Hard Coded Index Of Refraction -> 1.5f -> F0 -> 0.04)
    float Dr = gtr1(ndoth, lerp(0.1f, 0.001f, material_data.clearcoat_gloss)); // Normalized Isotropic GTR Gamma == 1
    float Fr = lerp(0.04f, 1.f, FH);
    float Gr = smith_gxx(0.25f, ndotl, ndotv);

    results.diffuse = (1.f / PI) * (lerp(Fd, ss, material_data.subsurface) * input.base_color + Fsheen) * (1.f - input.metallic) * (1 - F);
    results.specular = saturate(Ds * F * G);
    results.clearcoat = saturate(0.25f * material_data.clearcoat * Dr * Fr * Gr);

    return results;
} 

[shader("fragment")]
float4 frag_main(VertexOutput in) : SV_Target
{
    float2 uv = in.uv;
    float3 normalized_normal_ws = in.normal;
    float renorm_factor = 1.f / length(normalized_normal_ws);

    float3x3 world_to_tangent;
    float3 bitanget = cross(normalized_normal_ws, in.tangent.xyz) * in.tangent.w;
    world_to_tangent[0] = in.tangent.xyz * renorm_factor;
    world_to_tangent[1] = bitanget * renorm_factor;
    world_to_tangent[2] = normalized_normal_ws * renorm_factor;

    // Unpack tangent space normal
    float3 N;
    if (let texture = material_data.normal_texture.get())
    {
        float4 packed_normal = texture.Sample(in.uv);

        N.xy = packed_normal.xy * 2.0f - 1.0f;
        N.xy *= normal_strength;
        N.z = sqrt(saturate(1.0f - dot(N.xy, N.xy)));
        N = mul(N, world_to_tangent);
    }
    else
    {
        N = in.normal;
    }

    // Unpack DXT5nm tangent space tangent
    float3 T;
    if (let texture = material_data.tangent_texture.get())
    {
        T.xy = texture.Sample(in.uv).wy * 2.f - 1.f;
        T.z = sqrt(1 - saturate(dot(T.xy, T.xy)));
        T = mul(lerp(float3(1.0f, 0.0f, 0.0f), T, saturate(normal_strength)), world_to_tangent);
    }
    else
    {
        T = in.tangent.xyz;
    }

    float roughness;
    float metallic;
    if (let texture = material_data.metallic_roughness_texture.get())
    {
        float4 orm = texture.Sample(in.uv);
        roughness = material_data.roughness * orm.g;  // glTF: roughness in green channel
        metallic = material_data.metallic * orm.b;     // glTF: metallic in blue channel
    }
    else
    {
        roughness = material_data.roughness;
        metallic = material_data.metallic;
    }

    float3 albedo = material_data.color_texture.Sample(in.uv).rgb;
    albedo *= in.color;

    float3 camera_position = float3(scene_data.inverse_view[0][3], scene_data.inverse_view[1][3], scene_data.inverse_view[2][3]);

    BRDFInput input;
    input.N = N;
    input.L = normalize(scene_data.sunlight_direction.xyz);
    input.V = normalize(camera_position - in.world_position);
    input.X = T;
    input.Y = (cross(N, T) * in.tangent.w);
    input.roughness = roughness;
    input.metallic = metallic;
    input.base_color = albedo;

    BRDFResults reflection = disney_brdf(input);

    float3 output = scene_data.sunlight_color.rgb * (reflection.diffuse + reflection.specular + reflection.clearcoat);
    output *= saturate(dot(N, input.L));
    output = max(0.f, output);

    float3 ambient = albedo * scene_data.ambient_color.rgb;
    output += ambient;

    return float4(output, 1.0f);
}