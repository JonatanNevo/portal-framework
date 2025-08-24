module textures;

public interface ITexture
{
    Sampler2D get_texture();
    int get_uv_set();
    float3x3 get_uv_transform();
} 

public struct MaterialTexture: ITexture
{
    public Sampler2D texture;
    public int uv_set;
    public float3x3 uv_transform;

    public Sampler2D get_texture() { return texture; }
    public int get_uv_set() { return uv_set; }
    public float3x3 get_uv_transform() { return uv_transform; }
}

public struct ScaledTexture: ITexture
{
    public Sampler2D texture;
    public int uv_set;
    public float3x3 uv_transform;
    public float scale;

    public Sampler2D get_texture() { return texture; }
    public int get_uv_set() { return uv_set; }
    public float3x3 get_uv_transform() { return uv_transform; }
}

public struct FactoredTexture: ITexture
{
    public Sampler2D texture;
    public int uv_set;
    public float3x3 uv_transform;
    public float3 factor;

    public Sampler2D get_texture() { return texture; }
    public int get_uv_set() { return uv_set; }
    public float3x3 get_uv_transform() { return uv_transform; }
}

public float2 get_uv_transformed<T>(T texture, float2 v_texcoord_0, float2 v_texcoord_1) where T : ITexture
{
    float3 uv = float3(texture.get_uv_set() < 1 ? v_texcoord_0 : v_texcoord_1, 1.f);
    uv = mul(texture.get_uv_transform(), uv);
    return uv.xy;
}

public float2 get_uv<T>(T texture, float2 v_texcoord_0, float2 v_texcoord_1) where T : ITexture
{
    float3 uv = float3(texture.get_uv_set() < 1 ? v_texcoord_0 : v_texcoord_1, 1.f);
    return uv.xy;
}

public float2 get_normal_uv(ScaledTexture normal_texture, float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_NORMAL_UV_TRANSFORM
    return get_uv_transformed(normal_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(normal_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_emissive_uv(FactoredTexture emissive_texture, float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_EMISSIVE_UV_TRANSFORM
    return get_uv_transformed(emissive_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(emissive_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_occlusion_uv(ScaledTexture occlusion_texture, float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_OCCLUSION_UV_TRANSFORM
    return get_uv_transformed(occlusion_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(occlusion_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#ifdef MATERIAL_METALLICROUGHNESS

public float2 get_base_color_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_BASECOLOR_UV_TRANSFORM
    return get_uv_transformed(color_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(color_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_metallic_roughness_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_METALLICROUGHNESS_UV_TRANSFORM
    return get_uv_transformed(metallic_roughness_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(metallic_roughness_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#endif

#ifdef MATERIAL_SPECULARGLOSSINESS

public float2 get_specular_glossiness_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_SPECULARGLOSSINESS_UV_TRANSFORM
    return get_uv_transformed(specular_glossiness_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(specular_glossiness_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_diffuse_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_DIFFUSE_UV_TRANSFORM
    return get_uv_transformed(diffuse_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(diffuse_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#endif


#ifdef MATERIAL_CLEARCOAT
public float2 get_clearcoat_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_CLEARCOAT_UV_TRANSFORM
    return get_uv_transformed(clearcoat_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(clearcoat_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_clearcoat_roughness_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_CLEARCOATROUGHNESS_UV_TRANSFORM
    return get_uv_transformed(clearcoat_roughness_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(clearcoat_roughness_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_clearcoat_normal_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_CLEARCOATNORMAL_UV_TRANSFORM
    return get_uv_transformed(clearcoat_normal_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(clearcoat_normal_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#endif

#ifdef MATERIAL_SHEEN

public float2 get_sheen_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_SHEENCOLOR_UV_TRANSFORM
    return get_uv_transformed(sheen_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(sheen_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_sheen_roughness_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_SHEENROUGHNESS_UV_TRANSFORM
    return get_uv_transformed(sheen_roughness_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(sheen_roughness_texture, v_texcoord_0, v_texcoord_1);
#endif
}
#endif

#ifdef MATERIAL_SPECULAR

public float2 get_specular_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_SPECULAR_UV_TRANSFORM
    return get_uv_transformed(specular_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(specular_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_specular_color_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_SPECULARCOLOR_UV_TRANSFORM
    return get_uv_transformed(specular_color_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(specular_color_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#endif


#ifdef MATERIAL_TRANSMISSION

public struct TransmissionTexture
{
    public Sampler2D texture;
    public int uv_set;
    public float3x3 uv_transform;
    public Sampler2D framebuffer_sampler;
    public int2 framebuffer_size;
}

public float2 get_transmission_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_TRANSMISSION_UV_TRANSFORM
    return get_uv_transformed(transmission_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(transmission_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#endif


#ifdef MATERIAL_VOLUME

public float2 get_thickness_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_THICKNESS_UV_TRANSFORM
    return get_uv_transformed(thickness_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(thickness_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#endif

#ifdef MATERIAL_IRIDESCENCE

public float2 get_iridescence_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_IRIDESCENCE_UV_TRANSFORM
    return get_uv_transformed(iridescence_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(iridescence_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_iridescence_thickness_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_IRIDESCENCETHICKNESS_UV_TRANSFORM
    return get_uv_transformed(iridescence_thickness_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(iridescence_thickness_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#endif

#ifdef MATERIAL_DIFFUSE_TRANSMISSION

public float2 get_diffuse_transmission_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_DIFFUSETRANSMISSION_UV_TRANSFORM
    return get_uv_transformed(diffuse_transmission_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(diffuse_transmission_texture, v_texcoord_0, v_texcoord_1);
#endif
}

public float2 get_diffuse_transmission_color_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_DIFFUSETRANSMISSIONCOLOR_UV_TRANSFORM
    return get_uv_transformed(diffuse_transmission_color_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(diffuse_transmission_color_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#endif

#ifdef MATERIAL_ANISOTROPY

public float2 get_anisotropy_uv(float2 v_texcoord_0, float2 v_texcoord_1)
{
#ifdef HAS_ANISOTROPY_UV_TRANSFORM
    return get_uv_transformed(anisotropy_texture, v_texcoord_0, v_texcoord_1);
#else
    return get_uv(anisotropy_texture, v_texcoord_0, v_texcoord_1);
#endif
}

#endif