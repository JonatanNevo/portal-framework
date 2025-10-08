module functions;

public static const float M_PI = 3.141592653589793;

public extern static const bool has_normal_vec3;
public extern static const bool has_tangent_vec4;
public extern static const bool has_color;
public extern static const bool has_texcoord_0;
public extern static const bool has_texcoord_1;

static const bool has_tbn = has_normal_vec3 && has_tangent_vec4;

public struct Vertex
{
    public float3 position;

    public Conditional<float3x3, has_tbn> tbn;
    public Conditional<float4, has_tangent_vec4> tangent;

    public Conditional<float3, has_normal_vec3 && !has_tangent_vec4> normal;

    public Conditional<float4, has_color> color;

    public Conditional<float2, has_texcoord_0> uv0;
    public Conditional<float2, has_texcoord_1> uv1;
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