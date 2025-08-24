//
// Fresnel
//
// http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// https://github.com/wdas/brdf/tree/master/src/brdfs
// https://google.github.io/filament/Filament.md.html
//

module brdf;
import functions;


// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
float3 f_schlick(float3 f0, float3 f90, float v_dot_h)
{
    return f0 + (f90 - f0) * pow(clamp(1.0 - v_dot_h, 0.0, 1.0), 5.0);
}

float f_schlick(float f0, float f90, float v_dot_h)
{
    float x = clamp(1.0 - v_dot_h, 0.0, 1.0);
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return f0 + (f90 - f0) * x5;
}

float f_schlick(float f0, float v_dot_h)
{
    float f90 = 1.0; //clamp(50.0 * f0, 0.0, 1.0);
    return f_schlick(f0, f90, v_dot_h);
}

float3 f_schlick(float3 f0, float f90, float v_dot_h)
{
    float x = clamp(1.0 - v_dot_h, 0.0, 1.0);
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return f0 + (f90 - f0) * x5;
}

float3 f_schlick(float3 f0, float v_dot_h)
{
    float f90 = 1.0; //clamp(dot(f0, vec3(50.0 * 0.33)), 0.0, 1.0);
    return f_schlick(f0, f90, v_dot_h);
}

float3 schlick_to_f0(float3 f, float3 f90, float v_dot_h) 
{
    float x = clamp(1.0 - v_dot_h, 0.0, 1.0);
    float x2 = x * x;
    float x5 = clamp(x * x2 * x2, 0.0, 0.9999);

    return (f - f90 * x5) / (1.0 - x5);
}

float schlick_to_f0(float f, float f90, float v_dot_h) 
{
    float x = clamp(1.0 - v_dot_h, 0.0, 1.0);
    float x2 = x * x;
    float x5 = clamp(x * x2 * x2, 0.0, 0.9999);

    return (f - f90 * x5) / (1.0 - x5);
}

float3 schlick_to_f0(float3 f, float v_dot_h)
 {
    return schlick_to_f0(f, float3(1.0), v_dot_h);
}

float schlick_to_f0(float f, float v_dot_h) 
{
    return schlick_to_f0(f, 1.0, v_dot_h);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
public float V_GGX(float n_dot_l, float n_dot_v, float alpha_roughness)
{
    float alpha_roughness_sq = alpha_roughness * alpha_roughness;

    float GGXV = n_dot_l * sqrt(n_dot_v * n_dot_v * (1.0 - alpha_roughness_sq) + alpha_roughness_sq);
    float GGXL = n_dot_v * sqrt(n_dot_l * n_dot_l * (1.0 - alpha_roughness_sq) + alpha_roughness_sq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
public float D_GGX(float n_dot_h, float alpha_roughness)
{
    float alpha_roughness_sq = alpha_roughness * alpha_roughness;
    float f = (n_dot_h * n_dot_h) * (alpha_roughness_sq - 1.0) + 1.0;
    return alpha_roughness_sq / (M_PI * f * f);
}


float lambda_sheen_numeric_helper(float x, float alphaG)
{
    float one_minus_alpha_sq = (1.0 - alphaG) * (1.0 - alphaG);
    float a = lerp(21.5473, 25.3245, one_minus_alpha_sq);
    float b = lerp(3.82987, 3.32435, one_minus_alpha_sq);
    float c = lerp(0.19823, 0.16801, one_minus_alpha_sq);
    float d = lerp(-1.97760, -1.27393, one_minus_alpha_sq);
    float e = lerp(-4.32054, -4.85967, one_minus_alpha_sq);
    return a / (1.0 + b * pow(x, c)) + d * x + e;
}

float lambda_sheen(float cos_theta, float alpha_g)
{
     if (abs(cos_theta) < 0.5)
    {
        return exp(lambda_sheen_numeric_helper(cos_theta, alpha_g));
    }
    else
    {
        return exp(2.0 * lambda_sheen_numeric_helper(0.5, alpha_g) - lambda_sheen_numeric_helper(1.0 - cos_theta, alpha_g));
    }
}

float V_sheen(float n_dot_l, float n_dot_v, float sheen_roughness)
{
    sheen_roughness = max(sheen_roughness, 0.000001); //clamp (0,1]
    float alpha_g = sheen_roughness * sheen_roughness;

    return clamp(1.0 / ((1.0 + lambda_sheen(n_dot_v, alpha_g) + lambda_sheen(n_dot_l, alpha_g)) * (4.0 * n_dot_v * n_dot_l)), 0.0, 1.0);
}

//Sheen implementation-------------------------------------------------------------------------------------
// See  https://github.com/sebavan/glTF/tree/KHR_materials_sheen/extensions/2.0/Khronos/KHR_materials_sheen

// Estevez and Kulla http://www.aconty.com/pdf/s2017_pbs_imageworks_sheen.pdf
float D_charlie(float sheen_roughness, float n_dot_h)
{
    sheen_roughness = max(sheen_roughness, 0.000001); //clamp (0,1]
    float alpha_g = sheen_roughness * sheen_roughness;
    float inv_r = 1.0 / alpha_g;
    float cos2h = n_dot_h * n_dot_h;
    float sin2h = 1.0 - cos2h;
    return (2.0 + inv_r) * pow(sin2h, inv_r * 0.5) / (2.0 * M_PI);
}

//https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
public float3 BRDF_lambertian(float3 diffuse_color)
{
    // see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
    return (diffuse_color / M_PI);
}

//  https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
public float3 BRDF_specular_GGX(float alpha_roughness, float NdotL, float NdotV, float NdotH)
{
    float Vis = V_GGX(NdotL, NdotV, alpha_roughness);
    float D = D_GGX(NdotH, alpha_roughness);

    return float3(Vis * D);
}

#ifdef MATERIAL_ANISOTROPY
// GGX Distribution Anisotropic (Same as Babylon.js)
// https://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf Addenda

float D_GGX_anisotropic(float NdotH, float TdotH, float BdotH, float anisotropy, float at, float ab)
{
    float a2 = at * ab;
    float3 f = float3(ab * TdotH, at * BdotH, a2 * NdotH);
    float w2 = a2 / dot(f, f);
    return a2 * w2 * w2 / M_PI;
}

// GGX Mask/Shadowing Anisotropic (Same as Babylon.js - smithVisibility_GGXCorrelated_Anisotropic)
// Heitz http://jcgt.org/published/0003/02/03/paper.pdf
float V_GGX_anisotropic(float NdotL, float NdotV, float BdotV, float TdotV, float TdotL, float BdotL, float at, float ab)
{
    float GGXV = NdotL * length(float3(at * TdotV, ab * BdotV, NdotV));
    float GGXL = NdotV * length(float3(at * TdotL, ab * BdotL, NdotL));
    float v = 0.5 / (GGXV + GGXL);
    return clamp(v, 0.0, 1.0);
}

public float3 BRDF_specularGGXAnisotropy(float alphaRoughness, float anisotropy, float3 n, float3 v, float3 l, float3 h, float3 t, float3 b)
{
    // Roughness along the anisotropy bitangent is the material roughness, while the tangent roughness increases with anisotropy.
    float at = lerp(alphaRoughness, 1.0, anisotropy * anisotropy);
    float ab = clamp(alphaRoughness, 0.001, 1.0);

    float NdotL = clamp(dot(n, l), 0.0, 1.0);
    float NdotH = clamp(dot(n, h), 0.001, 1.0);
    float NdotV = dot(n, v);

    float V = V_GGX_anisotropic(NdotL, NdotV, dot(b, v), dot(t, v), dot(t, l), dot(b, l), at, ab);
    float D = D_GGX_anisotropic(NdotH, dot(t, h), dot(b, h), anisotropy, at, ab);

    return float3(V * D);
}
#endif

// f_sheen
public float3 BRDF_specular_sheen(float3 sheenColor, float sheenRoughness, float NdotL, float NdotV, float NdotH)
{
    float sheenDistribution = D_charlie(sheenRoughness, NdotH);
    float sheenVisibility = V_sheen(NdotL, NdotV, sheenRoughness);
    return sheenColor * sheenDistribution * sheenVisibility;
}