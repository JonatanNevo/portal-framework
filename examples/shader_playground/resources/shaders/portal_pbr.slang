module portal_pbr;

import portal_common;

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2
public float ndfggx(float cos_lh, float roughness)
{
    float alpha = roughness * roughness;
    float alpha_sq = alpha * alpha;

    float denom = (cos_lh * cos_lh) * (alpha_sq - 1.0f) + 1.0f;
    return alpha_sq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
public float ga_schlick_g1(float cos_theta, float k)
{
    return cos_theta / (cos_theta * (1.0f - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
public float ga_schlick_ggx(float cos_li, float ndotv, float roughness)
{
    float r = roughness + 1.f;
    float k = (r * r) / 8.f; // Epic suggests using this roughness remapping for analytic lights.
    return ga_schlick_g1(cos_li, k) * ga_schlick_g1(ndotv, k);
}

public float geometry_schlick_ggx(float ndotv, float roughness)
{
    float r = (roughness + 1.f);
    float k = (r * r) / 8.f;

    float nom = ndotv;
    float denom = ndotv * (1.f - k) + k;

    return nom / denom;
}

public float geometry_smith(float3 N, float3 V, float3 L, float roughness)
{
    float ndotv = max(dot(N, V), 0.f);
    float ndotl = max(dot(N, L), 0.f);
    float ggx2 = geometry_schlick_ggx(ndotv, roughness);
    float ggx1 = geometry_schlick_ggx(ndotl, roughness);

    return ggx1 * ggx2;
}

// Shlick's approximation of the Fresnel factor.
public float3 fresnel_schlick(float3 f0, float cos_theta)
{
    return f0 + (1.f - f0) * pow(1.f - cos_theta, 5.f);
}

public float3 fresnel_schlick_roughness(float3 f0, float cos_theta, float roughness)
{
    return f0 + (max(float3(1.f - roughness), f0) - f0) * pow(1.f - cos_theta, 5.f);
}