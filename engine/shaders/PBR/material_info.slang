module material_info;

import functions;
import textures;
import ibl;

public struct MaterialInputs
{
    float metallic_factor;
    float roughness_factor;
    float4 base_color_factor;

    float sheen_roughness_factor;
    float sheen_color_factor;

    float clearcoat_factor;
    float clearcoat_roughness_factor;

    float3 specular_color_factor;
    float specular_factor;

    float transmission_factor;

    float thickness_factor;
    float3 attenuation_color;
    float attenuation_distance;

    float iridescence_factor;
    float iridescence_ior;
    float iridescence_thickness_min;
    float iridescence_thickness_max;

    float diffuse_transmission_factor;
    float3 diffuse_transmission_color_factor;

    public float emissive_strength;

    float ior;

    float3 anisotropy;

    float dispersion;

    public float alpha_cutoff;
}

public struct SceneData
{
    public float3 camera;
    public float4x4 view_matrix;
    public float4x4 projection_matrix;
    public float4x4 view_proj;
    public int2 screen_size;
    public float exposure; // TODO: have it in some camera struct?
}

public struct ModelInfo
{
    public float4x4 model_matrix;
    public float4x4 normal_matrix;
}

public struct Material
{
    public float ior;
    public float perceptual_roughness;
    public float3 f0_dielectric;

    public float alpha_roughness;

    public float fresnel_w;

    public float3 f90;
    public float3 f90_dielectric;
    public float metallic;

    public float3 base_color;
    
    public float sheen_roughness_factor;
    public float3 sheen_color_factor;

    public float3 clearcoat_f0;
    public float3 clearcoat_f90;
    public float clearcoat_factor;
    public float3 clearcoat_normal;
    public float clearcoat_roughness;

    public float specular_weight;

    public float transmission_factor;

    public float thickness;
    public float3 attenuation_color;
    public float attenuation_distance;

    public float iridescence_factor;
    public float iridescence_ior;
    public float iridescence_thickness;

    public float diffuse_transmission_factor;
    public float3 diffuse_transmission_color_factor;

    public float3 anisotropic_t;
    public float3 anisotropic_b;
    public float anisotropy_strength;

    public float dispersion;
}

public NormalInfo get_normal_info(Vertex vert, ScaledTexture normal_texture, float normal_scale, float3 v, float2 v_texcoord_0, float2 v_texcoord_1)
{
    float2 uv = get_normal_uv(normal_texture, v_texcoord_0, v_texcoord_1);
    float2 uv_dx = ddx(uv);
    float2 uv_dy = ddy(uv);

    if (length(uv_dx) < 1e-2)
    {
        uv_dx = float2(1, 0.0);
    }

    if (length(uv_dy) < 1e-2)
    {
        uv_dy = float2(0.0, 1);
    }

    float3 t_ =  (uv_dy.x * ddx(vert.position) - uv_dx.x * ddy(vert.position)) / (uv_dx.y * uv_dy.x - uv_dy.y * uv_dx.x);

    float3 n, t, b, ng;
// Compute geometrical TBN:
#ifdef HAS_NORMAL_VEC3
#ifdef HAS_TANGENT_VEC4
    // Trivial TBN computation, present as vertex attribute.
    // Normalize eigenvectors as matrix is linearly interpolated.
    t = normalize(vert.tbn[0]);
    b = normalize(vert.tbn[1]);
    ng = normalize(vert.tbn[2]);
#else
    // Normals are either present as vertex attributes or approximated.
    ng = normalize(vert.normal);
    t = normalize(t_ - ng * dot(ng, t_));
    b = cross(ng, t);
#endif
#else
    ng = normalize(cross(ddx(vert.position), ddy(vert.position)));
    t = normalize(t_ - ng * dot(ng, t_));
    b = cross(ng, t);
#endif

    NormalInfo info;
    info.geometry_normal = ng;

#ifdef HAS_NORMAL_MAP
    info.normal_tex = normal_texture.texture.Sample(uv).rgb * 2.0 - float3(1.0);
    info.normal_tex *= float3(normal_scale, normal_scale, 1.0);
    info.normal_tex = normalize(info.normal_tex);
    info.normal = normalize(mul(float3x3(t, b, ng), info.normal_tex));
#else
    info.normal = ng;
#endif

    info.tangent = t;
    info.bitangent = b;

    return info;
}

#ifdef MATERIAL_CLEARCOAT
public float3 get_clearcoat_normal(NormalInfo normal_info)
{
#ifdef HAS_CLEARCOAT_NORMAL_MAP
    float3 n = clearcoat_normal_texture.Sample(get_clearcoat_normal_uv()).rgb * 2.0 - float3(1.0);
    n *= float3(clearcoat_normal_scale, u_ClearcoatNormalScale, 1.0);
    n = mul(float3x3(normal_info.tangent, normal_info.bitangent, normal_info.geometry_normal), normalize(n));
    return n;
#else
    return normal_info.geometry_normal;
#endif
}
#endif

public float4 get_base_color(MaterialInputs input, float2 v_texcoord_0, float2 v_texcoord_1)
{
    float4 base_color = input.base_color_factor;

#ifdef HAS_BASE_COLOR_MAP
    base_color *= color_texture.Sample(get_base_color_uv(v_texcoord_0, v_texcoord_1));
#endif

    return base_color;
}

#ifdef MATERIAL_METALLICROUGHNESS
public Material get_metallic_roughness_info(MaterialInputs input, Material info, float2 v_texcoord_0, float2 v_texcoord_1)
{
    info.metallic = input.metallic_factor;
    info.perceptual_roughness = input.roughness_factor;

#ifdef HAS_METALLIC_ROUGHNESS_MAP
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
    float4 mr_sample = metallic_roughness_texture.texture.Sample(get_metallic_roughness_uv(v_texcoord_0, v_texcoord_1));
    info.perceptual_roughness *= mr_sample.g;
    info.metallic *= mr_sample.b;
#endif

    return info;
}
#endif

#ifdef MATERIAL_SHEEN
public Material get_sheen_info(MaterialInputs input, Material info, float2 v_texcoord_0, float2 v_texcoord_1)
{
    info.sheen_color_factor = input.sheen_color_factor;
    info.sheen_roughness_factor = input.sheen_roughness_factor;

#ifdef HAS_SHEEN_COLOR_MAP
    float4 sheen_color_sample = sheen_texture.texture.Sample(get_sheen_uv(v_texcoord_0, v_texcoord_1));
    info.sheen_color_factor *= sheen_color_sample.rgb;
#endif

#ifdef HAS_SHEEN_ROUGHNESS_MAP
    float4 sheen_roughness_sample = sheen_roughness_texture.texture.Sample(get_sheen_roughness_uv(v_texcoord_0, v_texcoord_1));
    info.sheen_roughness_factor *= sheen_roughness_sample.a;
#endif

    return info;
}
#endif

#ifdef MATERIAL_SPECULAR
public Material get_specular_info(MaterialInputs input, Material info, float2 v_texcoord_0, float2 v_texcoord_1)
{
    float4 specular_texture_sample = float4(1.0);
#ifdef HAS_SPECULAR_MAP
    specular_texture_sample.a = specular_texture.texture.Sample(get_specular_uv(v_texcoord_0, v_texcoord_1)).a;
#endif
#ifdef HAS_SPECULAR_COLOR_MAP
    specular_texture_sample.rgb = specular_color_texture.texture.Sample(get_specular_color_uv(v_texcoord_0, v_texcoord_1)).rgb;
#endif

    info.f0_dielectric = min(info.f0_dielectric * input.specular_color_factor * specular_texture_sample.rgb, float3(1.0));
    info.specular_weight = input.specular_factor * specular_texture_sample.a;
    info.f90_dielectric = float3(info.specular_weight);
    return info;
}
#endif

#ifdef MATERIAL_TRANSMISSION
public Material get_transmission_info(MaterialInputs input, Material info, float2 v_texcoord_0, float2 v_texcoord_1)
{
    info.transmission_factor = input.transmission_factor;

#ifdef HAS_TRANSMISSION_MAP
    float4 transmission_sample = transmission_texture.texture.Sample(get_transmission_uv(v_texcoord_0, v_texcoord_1));
    info.transmission_factor *= transmission_sample.r;
#endif

#ifdef MATERIAL_DISPERSION
    info.dispersion = input.dispersion;
#else
    info.dispersion = 0.0;
#endif
    return info;
}
#endif

#ifdef MATERIAL_VOLUME
public Material get_volume_info(MaterialInputs input, Material info, float2 v_texcoord_0, float2 v_texcoord_1)
{
    info.thickness = input.thickness_factor;
    info.attenuation_color = input.attenuation_color;
    info.attenuation_distance = input.attenuation_distance;

#ifdef HAS_THICKNESS_MAP
    float4 thickness_sample = thickness_texture.texture.Sample(get_thickness_uv(v_texcoord_0, v_texcoord_1));
    info.thickness *= thickness_sample.g;
#endif
    return info;
}
#endif

#ifdef MATERIAL_IRIDESCENCE
public Material get_iridescence_info(MaterialInputs input, Material info, float2 v_texcoord_0, float2 v_texcoord_1)
{
    info.iridescence_factor = input.iridescence_factor;
    info.iridescence_ior = input.iridescence_ior;
    info.iridescence_thickness = input.iridescence_thickness_max;

#ifdef HAS_IRIDESCENCE_MAP
    info.iridescence_factor *= iridescence_texture.texture.Sample(get_iridescence_uv(v_texcoord_0, v_texcoord_1)).r;
#endif

#ifdef HAS_IRIDESCENCE_THICKNESS_MAP
    float thickness_sampled = iridescence_thickness_texture.texture.Sample(get_iridescence_thickness_uv(v_texcoord_0, v_texcoord_1)).g;
    float thickness = lerp(input.iridescence_thickness_min, input.iridescence_thickness_max, thickness_sampled);
    info.iridescence_thickness = thickness;
#endif

    return info;
}
#endif

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
public Material get_diffuse_transmission_info(MaterialInputs input, Material info, float2 v_texcoord_0, float2 v_texcoord_1)
{
    info.diffuse_transmission_factor = input.diffuse_transmission_factor;
    info.diffuse_transmission_color_factor = input.diffuse_transmission_color_factor;

#ifdef HAS_DIFFUSE_TRANSMISSION_MAP
    info.diffuse_transmission_factor *= diffuse_transmission_texture.texture.Sample(get_diffuse_transmission_uv(v_texcoord_0, v_texcoord_1)).a;
#endif

#ifdef HAS_DIFFUSE_TRANSMISSION_COLOR_MAP
    info.diffuse_transmission_color_factor *= diffuse_transmission_color_texture.texture.Sample(get_diffuse_transmission_color_uv(v_texcoord_0, v_texcoord_1)).rgb;
#endif

    return info;
}
#endif

#ifdef MATERIAL_CLEARCOAT
public Material get_clearcoat_info(MaterialInputs input, Material info, NormalInfo normal_info, float2 v_texcoord_0, float2 v_texcoord_1)
{
    info.clearcoat_factor = input.clearcoat_factor;
    info.clearcoat_roughness = input.clearcoat_roughness_factor;
    info.clearcoat_f0 = float3(pow((info.ior - 1.0) / (info.ior + 1.0), 2.0));
    info.clearcoat_f90 = float3(1.0);

#ifdef HAS_CLEARCOAT_MAP
    float4 clearcoat_sample = clearcoat_texture.texture.Sample(get_clearcoat_uv(v_texcoord_0, v_texcoord_1));
    info.clearcoat_factor *= clearcoat_sample.r;
#endif

#ifdef HAS_CLEARCOAT_ROUGHNESS_MAP
    float4 clearcoat_sample_roughness = clearcoat_roughness_texture.texture.Sample(get_clearcoat_roughness_uv(v_texcoord_0, v_texcoord_1));
    info.clearcoat_roughness *= clearcoat_sample_roughness.g;
#endif

    info.clearcoat_normal = get_clearcoat_normal(normal_info);
    info.clearcoat_roughness = clamp(info.clearcoat_roughness, 0.0, 1.0);
    return info;
}
#endif

#ifdef MATERIAL_IOR
public Material get_ior_info(MaterialInputs input, Material info)
{
    info.f0_dielectric = float3(pow((input.ior - 1.0) / (input.ior + 1.0), 2.0));
    info.ior = input.ior;
    return info;
}
#endif

#ifdef MATERIAL_ANISOTROPY
public Material get_anisotropy_info(MaterialInputs input, Material info, NormalInfo normal_info, float2 v_texcoord_0, float2 v_texcoord_1)
{
    float2 direction = float2(1.0, 0.0);
    float strength_factor = 1.0;
#ifdef HAS_ANISOTROPY_MAP
    float3 anisotropy_sample = anisotropy_texture.texture.Sample(get_anisotropy_uv(v_texcoord_0, v_texcoord_1)).xyz;
    direction = anisotropy_sample.xy * 2.0 - float2(1.0);
    strength_factor = anisotropy_sample.z;
#endif
    float2 direction_rotation = input.anisotropy.xy; // cos(theta), sin(theta)
    float2x2 rotation_matrix = float2x2(direction_rotation.x, direction_rotation.y, -direction_rotation.y, direction_rotation.x);
    direction = mul(rotation_matrix, direction.xy);

    info.anisotropic_t = mul(float3x3(normal_info.tangent, normal_info.bitangent, normal_info.normal), normalize(float3(direction, 0.0)));
    info.anisotropic_b = cross(normal_info.geometry_normal, info.anisotropic_t);
    info.anisotropy_strength = clamp(input.anisotropy.z * strength_factor, 0.0, 1.0);
    return info;
}
#endif

public float albedo_sheen_scaling_lut(ImageBasedLightingData ibl, float ndot_v, float sheen_roughness_factor)
{
    return ibl.sheen_energy_lookup_table.Sample(float2(ndot_v, sheen_roughness_factor)).r;
}
