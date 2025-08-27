module functions;

public static const float M_PI = 3.141592653589793;

public struct Vertex
{
    public float3 position;

    #ifdef HAS_NORMAL_VEC3
    #ifdef HAS_TANGENT_VEC4
    public float3x3 tbn;
    public float4 tangent;
    #else
    public float3 normal;
    #endif
    #endif

    #ifdef HAS_COLOR_0_VEC3
    public float3 color;
    #endif
    #ifdef HAS_COLOR_0_VEC4
    public float4 color;
    #endif

    #ifdef HAS_TEXCOORD_0_VEC2
    public float2 uv0;
    #endif

    #ifdef HAS_TEXCOORD_1_VEC2
    public float2 uv1;
    #endif
}

public float4 get_vertex_color(Vertex vert)
{
    float4 color = float4(1.f);

#ifdef HAS_COLOR_0_VEC3
    color.rgb = vert.color;
#endif
#ifdef HAS_COLOR_0_VEC4
    color = vert.color;
#endif

    return color;
}

public struct NormalInfo
{
    public float3 geometry_normal;
    public float3 tangent;
    public float3 bitangent;
    public float3 normal;
    public float3 normal_tex; // Normal from texture, scaling is accounted for.
}

public float clamped_dot(float3 x, float3 y)
{
    return clamp(dot(x, y), 0.0f, 1.0f);
}

public float max3(float3 v)
{
    return max(max(v.x, v.y), v.z);
}

public float sq(float t)
{
    return t * t;
}

public float2 sq(float2 t)
{
    return t * t;
}

public float3 sq(float3 t)
{
    return t * t;
}

public float4 sq(float4 t)
{
    return t * t;
}

public float apply_ior_to_roughness(float roughness, float ior)
{
    // Scale roughness with IOR so that an IOR of 1.0 results in no microfacet refraction and
    // an IOR of 1.5 results in the default amount of microfacet refraction.
    return roughness * clamp(ior * 2.0 - 2.0, 0.0, 1.0);
}

public float3 rgb_mix(float3 base, float3 layer, float3 rgb_alpha)
{
    float rgb_alpha_max = max(rgb_alpha.r, max(rgb_alpha.g, rgb_alpha.b));
    return (1.0 - rgb_alpha_max) * base + rgb_alpha * layer;
}