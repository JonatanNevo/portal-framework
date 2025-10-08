module tonemapping;

enum ToneMapping
{
    NONE = 0,
    ACES_NARKOWICZ = 1,           // TONEMAP_ACES_NARKOWICZ
    ACES_HILL = 2,                // TONEMAP_ACES_HILL
    ACES_HILL_EXPOSURE_BOOST = 3, // TONEMAP_ACES_HILL_EXPOSURE_BOOST
    KHR_PBR_NEUTRAL = 4           // TONEMAP_KHR_PBR_NEUTRAL 
}

extern static const ToneMapping tone_mapping = ToneMapping.NONE;

static const float GAMMA = 2.2;
static const float INVERSE_GAMMA = 1.0 / GAMMA;


// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat = float3x3
(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat = float3x3
(
    1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602
);

// linear to sRGB approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
public float3 linear_to_sRGB(float3 color)
{
    return pow(color, float3(INVERSE_GAMMA));
}

// sRGB to linear approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
public float3 sRGB_to_linear(float3 srgb)
{
    return pow(srgb.xyz, float3(GAMMA));
}

public float4 sRGBToLinear(float4 srgb)
{
    return float4(sRGB_to_linear(srgb.xyz), srgb.w);
}

// ACES tone map (faster approximation)
// see: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
public float3 tonemap_aces_markowicz(float3 color)
{
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}

// ACES filmic tone map approximation
// see https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
public float3 rrt_and_odt_fit(float3 color)
{
    float3 a = color * (color + 0.0245786) - 0.000090537;
    float3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    return a / b;
}

// tone mapping
public float3 tonemap_aces_hill(float3 color)
{
     color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = rrt_and_odt_fit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

// Khronos PBR neutral tone mapping
public float3 tonemap_khronos_pbr_neutral(float3 color)
{
    const float start_compression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < start_compression) return color;

    const float d = 1. - start_compression;
    float newPeak = 1. - d * d / (peak + d - start_compression);
    color *= newPeak / peak;

    float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
    return lerp(color, newPeak * float3(1, 1, 1), g);
}

public float3 tonemap(float exposure, float3 color)
{
    color *= exposure;

    switch (tone_mapping)
    {
        case ToneMapping.NONE:
            break;
        case ToneMapping.ACES_NARKOWICZ:
            color = tonemap_aces_markowicz(color);
            break;
        case ToneMapping.ACES_HILL:
            color = tonemap_aces_hill(color);
            break;
        case ToneMapping.ACES_HILL_EXPOSURE_BOOST:
            // boost exposure as discussed in https://github.com/mrdoob/three.js/pull/19621
            // this factor is based on the exposure correction of Krzysztof Narkowicz in his
            // implemetation of ACES tone mapping
            color /= 0.6;
            color = tonemap_aces_hill(color);
            break;
        case ToneMapping.KHR_PBR_NEUTRAL:
            color = tonemap_khronos_pbr_neutral(color);
            break;

    }
    
    return linear_to_sRGB(color);
}