module material_info;

import functions;
import textures;
import ibl;
import common;

public extern static const bool has_normal_map;
public extern static const bool has_dispersion;

public struct SceneData
{
    public float3 camera;
    public float4x4 view_matrix;
    public float4x4 projection_matrix;
    public float4x4 view_proj;
    public int2 screen_size;
    public float exposure; // TODO: have it in some camera struct?
}

public struct NormalInfo
{
    public float3 geometry_normal;
    public float3 tangent;
    public float3 bitangent;
    public float3 normal;
    public float3 normal_tex; // Normal from texture, scaling is accounted for.

    public float3 get_clearcoat_normal(in Textures textures, in Array<float2, uv_set_number> uvs)
    {
        if (let clearcoat_normal_texture = textures.clearcoat_normal_texture.get())
        {
            float3 n = clearcoat_normal_texture.texture.Sample(clearcoat_normal_texture.get_uv(uvs)).rgb * 2.0 - float3(1.0);
            n *= float3(textures.normal_scale, textures.clearcoat_normal_scale.get().value, 1.0);
            n = mul(float3x3(tangent, bitangent, geometry_normal), normalize(n));
            return n;
        }
        return geometry_normal;
    }
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

    Conditional<float, has_dispersion> dispersion;

    public float alpha_cutoff;

    public float4 get_base_color(in Textures textures, in Array<float2, uv_set_number> uvs)
    {
        float4 base_color = base_color_factor;

        if (let color_texture = textures.color_texture.get())
        {
            base_color *= color_texture.texture.Sample(color_texture.get_uv(uvs));
        }

        return base_color;
    }

    public void get_metallic_roughness_info(in Textures textures, in Array<float2, uv_set_number> uvs, out Material info)
    {
        info.metallic = metallic_factor;
        info.perceptual_roughness = roughness_factor;

        if (let metallic_texture = textures.metallic_roughness_texture.get())
        {
            // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
            // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
            float4 mr_sample = metallic_texture.texture.Sample(metallic_texture.get_uv(uvs));
            info.perceptual_roughness *= mr_sample.g;
            info.metallic *= mr_sample.b;
        }
    }

    public Material get_sheen_info(in Textures textures, in Array<float2, uv_set_number> uvs, out Material info)
    {
        info.sheen_color_factor = sheen_color_factor;
        info.sheen_roughness_factor = sheen_roughness_factor;

        if (let sheen_texture = textures.sheen_texture.get())
        {
            float4 sheen_color_sample = sheen_texture.texture.Sample(sheen_texture.get_uv(uvs));
            info.sheen_color_factor *= sheen_color_sample.rgb;
        }

        if (let sheen_roughness_texture = textures.sheen_roughness_texture.get())
        {
            float4 sheen_roughness_sample = sheen_roughness_texture.texture.Sample(sheen_roughness_texture.get_uv(uvs));
            info.sheen_roughness_factor *= sheen_roughness_sample.a;
        }

        return info;
    }

    public Material get_specular_info(in Textures textures, in Array<float2, uv_set_number> uvs, out Material info)
    {
        float4 specular_texture_sample = float4(1.0);

        if (let specular_texture = textures.specular_glossiness_texture.get())
        {
            specular_texture_sample = specular_texture.texture.Sample(specular_texture.get_uv(uvs));
        }

        if (let specular_color_texture = textures.specular_color_texture.get())
        {
            specular_texture_sample.rgb = specular_color_texture.texture.Sample(specular_color_texture.get_uv(uvs)).rgb;
        }

        info.f0_dielectric = min(info.f0_dielectric * specular_color_factor * specular_texture_sample.rgb, float3(1.0));
        info.specular_weight = specular_factor * specular_texture_sample.a;
        info.f90_dielectric = float3(info.specular_weight);

        return info;
    }

    public Material get_transmission_info(in Textures textures, in Array<float2, uv_set_number> uvs, out Material info)
    {
        info.transmission_factor = transmission_factor;

        if (let transmission_texture = textures.transmission_texture.get())
        {
            float4 transmission_sample = transmission_texture.texture.texture.Sample(transmission_texture.texture.get_uv(uvs));
            info.transmission_factor *= transmission_sample.r;
        }

        if (let value = dispersion.get())
        {
            info.dispersion = value;
        }
        else
        {
            info.dispersion = 0.0;
        }

        return info;
    }

    public Material get_volume_info(in Textures textures, in Array<float2, uv_set_number> uvs, out Material info)
    {
        info.thickness = thickness_factor;
        info.attenuation_color = attenuation_color;
        info.attenuation_distance = attenuation_distance;

        if (let thickness_texture = textures.thickness_texture.get())
        {
            float4 thickness_sample = thickness_texture.texture.Sample(thickness_texture.get_uv(uvs));
            info.thickness *= thickness_sample.g;
        }
        return info;
    }

    public Material get_iridescence_info(in Textures textures, in Array<float2, uv_set_number> uvs, out Material info)
    {
        info.iridescence_factor = iridescence_factor;
        info.iridescence_ior = iridescence_ior;
        info.iridescence_thickness = iridescence_thickness_max;

        if (let iridescence_texture = textures.iridescence_texture.get())
        {
            info.iridescence_factor *= iridescence_texture.texture.Sample(iridescence_texture.get_uv(uvs)).r;
        }

        if (let iridescence_thickness_texture = textures.iridescence_thickness_texture.get())
        {
            float thickness_sampled = iridescence_thickness_texture.texture.Sample(iridescence_thickness_texture.get_uv(uvs)).g;
            float thickness = lerp(iridescence_thickness_min, iridescence_thickness_max, thickness_sampled);
            info.iridescence_thickness = thickness;
        }

        return info;
    }

    public Material get_diffuse_transmission_info(in Textures textures, in Array<float2, uv_set_number> uvs, out Material info)
    {
        info.diffuse_transmission_factor = diffuse_transmission_factor;
        info.diffuse_transmission_color_factor = diffuse_transmission_color_factor;

        if (let diffuse_transmission_texture = textures.diffuse_transmission_texture.get())
        {
            info.diffuse_transmission_factor *= diffuse_transmission_texture.texture.Sample(diffuse_transmission_texture.get_uv(uvs)).a;
        }

        if (let diffuse_transmission_color_texture = textures.diffuse_transmission_color_texture.get())
        {
            info.diffuse_transmission_color_factor *= diffuse_transmission_color_texture.texture.Sample(diffuse_transmission_color_texture.get_uv(uvs)).rgb;
        }

        return info;
    }

    public Material get_clearcoat_info(in Textures textures, in Array<float2, uv_set_number> uvs, in NormalInfo normal_info, out Material info)
    {
        info.clearcoat_factor = clearcoat_factor;
        info.clearcoat_roughness = clearcoat_roughness_factor;
        info.clearcoat_f0 = float3(pow((info.ior - 1.0) / (info.ior + 1.0), 2.0));
        info.clearcoat_f90 = float3(1.0);

        if (let clearcoat_texture = textures.clearcoat_texture.get())
        {
            float4 clearcoat_sample = clearcoat_texture.texture.Sample(clearcoat_texture.get_uv(uvs));
            info.clearcoat_factor *= clearcoat_sample.r;
        }

        if (let clearcoat_roughness_texture = textures.clearcoat_roughness_texture.get())
        {
            float4 clearcoat_sample_roughness = clearcoat_roughness_texture.texture.Sample(clearcoat_roughness_texture.get_uv(uvs));
            info.clearcoat_roughness *= clearcoat_sample_roughness.g;
        }

        info.clearcoat_normal = normal_info.get_clearcoat_normal(textures, uvs);
        info.clearcoat_roughness = clamp(info.clearcoat_roughness, 0.0, 1.0);
        return info;
    }

    public Material get_ior_info(out Material info)
    {
        info.f0_dielectric = float3(pow((ior - 1.0) / (ior + 1.0), 2.0));
        info.ior = ior;
        return info;
    }

    public Material get_anisotropy_info(in Textures textures, in Array<float2, uv_set_number> uvs, in NormalInfo normal_info, out Material info)
    {
        float2 direction = float2(1.0, 0.0);
        float strength_factor = 1.0;

        if (let anisotropy_texture = textures.anisotropy_texture.get())
        {
            float3 anisotropy_sample = anisotropy_texture.texture.Sample(anisotropy_texture.get_uv(uvs)).xyz;
            direction = anisotropy_sample.xy * 2.0 - float2(1.0);
            strength_factor = anisotropy_sample.z;
        }

        float2 direction_rotation = anisotropy.xy; // cos(theta), sin(theta)
        float2x2 rotation_matrix = float2x2(direction_rotation.x, direction_rotation.y, -direction_rotation.y, direction_rotation.x);
        direction = mul(rotation_matrix, direction.xy);

        info.anisotropic_t = mul(float3x3(normal_info.tangent, normal_info.bitangent, normal_info.normal), normalize(float3(direction, 0.0)));
        info.anisotropic_b = cross(normal_info.geometry_normal, info.anisotropic_t);
        info.anisotropy_strength = clamp(anisotropy.z * strength_factor, 0.0, 1.0);
        return info;
    }
}

public NormalInfo get_normal_info(Vertex vert, in Textures textures, float3 v, in Array<float2, uv_set_number> uvs)
{
    Texture normal_texture = textures.normal_texture;
    float2 uv = uvs[normal_texture.uv_index];
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

    if (has_normal_vec3)
    {
        if (has_tangent_vec4)
        {
            // Trivial TBN computation, present as vertex attribute.
            // Normalize eigenvectors as matrix is linearly interpolated.
            t = normalize(vert.tbn.get().value[0]);
            b = normalize(vert.tbn.get().value[1]);
            ng = normalize(vert.tbn.get().value[2]);
        }
        else
        {
            // Normals are either present as vertex attributes or approximated.
            ng = normalize(vert.normal.get().value);
            t = normalize(t_ - ng * dot(ng, t_));
            b = cross(ng, t);
        }
    }
    else
    {
        ng = normalize(cross(ddx(vert.position), ddy(vert.position)));
        t = normalize(t_ - ng * dot(ng, t_));
        b = cross(ng, t);
    }

    NormalInfo info;
    info.geometry_normal = ng;

    if (has_normal_map)
    {
        info.normal_tex = normal_texture.texture.Sample(uv).rgb * 2.0 - float3(1.0);
        info.normal_tex *= float3(textures.normal_scale, textures.normal_scale, 1.0);
        info.normal_tex = normalize(info.normal_tex);
        info.normal = normalize(mul(float3x3(t, b, ng), info.normal_tex));
    }
    else
    {
        info.normal = ng;
    }

    info.tangent = t;
    info.bitangent = b;

    return info;
}