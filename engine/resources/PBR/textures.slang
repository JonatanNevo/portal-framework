module textures;
import common;

public extern const static bool has_metalic_roughness;
public extern const static bool has_specular_glossiness;
public extern const static bool has_clearcoat;
public extern static const bool has_clearcoat_normal_map;
public extern const static bool has_sheen;
public extern const static bool has_specular;
public extern const static bool has_transmission;
public extern const static bool has_volume;
public extern const static bool has_iridescence;
public extern const static bool has_diffuse_transmission;
public extern const static bool has_anisotropy;

public struct Texture
{
    public Sampler2D texture;
    public int uv_index;

    public float2 get_uv(Array<float2, uv_set_number> uvs)
    {
        return uvs[uv_index];
    }
}

public struct TransmissionTexture
{
    public Sampler2D framebuffer_sampler;
    public int2 framebuffer_size;

    public Texture texture;
}

public struct Textures
{
    public Texture normal_texture;
    public float normal_scale;
    public Texture emissive_texture;
    public float3 emissive_factor;
    public Texture occlusion_texture;
    public float occlusion_scale;

    // MATERIAL_METALLICROUGHNESS
    public Conditional<Texture, has_metalic_roughness> color_texture;
    public Conditional<Texture, has_metalic_roughness> metallic_roughness_texture;

    // MATERIAL_SPECULARGLOSSINESS
    public Conditional<Texture, has_specular_glossiness> specular_glossiness_texture;
    public Conditional<Texture, has_specular_glossiness> diffuse_texture;

    // MATERIAL_CLEARCOAT
    public Conditional<Texture, has_clearcoat> clearcoat_texture;
    public Conditional<Texture, has_clearcoat> clearcoat_roughness_texture;
    public Conditional<Texture, has_clearcoat && has_clearcoat_normal_map> clearcoat_normal_texture;
    public Conditional<float, has_clearcoat> clearcoat_normal_scale;

    // MATERIAL_SHEEN
    public Conditional<Texture, has_sheen> sheen_texture;
    public Conditional<Texture, has_sheen> sheen_roughness_texture;

    // MATERIAL_SPECULAR
    public Conditional<Texture, has_specular> specular_texture;
    public Conditional<Texture, has_specular> specular_color_texture;

    // MATERIAL_TRANSMISSION
    public Conditional<TransmissionTexture, has_transmission> transmission_texture;

    // MATERIAL_VOLUME
    public Conditional<Texture, has_volume> thickness_texture;

    // MATERIAL_IRIDESCENCE
    public Conditional<Texture, has_iridescence> iridescence_texture;
    public Conditional<Texture, has_iridescence> iridescence_thickness_texture;

    // MATERIAL_DIFFUSE_TRANSMISSION
    public Conditional<Texture, has_diffuse_transmission> diffuse_transmission_texture;
    public Conditional<Texture, has_diffuse_transmission> diffuse_transmission_color_texture;

    // MATERIAL_ANISOTROPY
    public Conditional<Texture, has_anisotropy> anisotropy_texture;
}