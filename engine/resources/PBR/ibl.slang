module ibl;

import textures;
import functions;
import lights;

public struct ImageBasedLightingData
{
    public int mip_count;
    public SamplerCube lambertial_sampler;
    public SamplerCube ggx_sampler;
    public Sampler2D ggx_lookup_table;
    public SamplerCube charlie_sampler;
    public Sampler2D charlie_lookup_table;
    public Sampler2D sheen_energy_lookup_table;
    public float3x3 rotation;
    public float intensity;

    public float albedo_sheen_scaling_lut(float ndot_v, float sheen_roughness_factor)
    {
        return sheen_energy_lookup_table.Sample(float2(ndot_v, sheen_roughness_factor)).r;
    }
}


float3 get_diffuse_light(ImageBasedLightingData ibl, float3 n)
{
    float4 texture_sample = ibl.lambertial_sampler.Sample(mul(ibl.rotation, n));
    texture_sample.rgb *= ibl.intensity;
    return texture_sample.rgb;
}

float4 get_specular_sample(ImageBasedLightingData ibl, float3 reflection, float lod)
{
    float4 texture_sample = ibl.ggx_sampler.SampleLevel(mul(ibl.rotation, reflection), lod);
    texture_sample.rgb *= ibl.intensity;
    return texture_sample;
}

float4 get_sheen_sample(ImageBasedLightingData ibl, float3 reflection, float lod)
{
    float4 texture_sample = ibl.charlie_sampler.SampleLevel(mul(ibl.rotation, reflection), lod);
    texture_sample.rgb *= ibl.intensity;
    return texture_sample;
}

float3 get_ibl_ggx_fresnel(ImageBasedLightingData ibl, float3 n, float3 v, float roughness, float3 f0, float specular_weight)
{
    // see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
    // Roughness dependent fresnel, from Fdez-Aguera
    float NdotV = clamped_dot(n, v);
    float2 brdfSamplePoint = clamp(float2(NdotV, roughness), float2(0.0, 0.0), float2(1.0, 1.0));
    float2 f_ab = ibl.ggx_lookup_table.Sample(brdfSamplePoint).rg;
    float3 Fr = max(float3(1.0 - roughness), f0) - f0;
    float3 k_S = f0 + Fr * pow(1.0 - NdotV, 5.0);
    float3 FssEss = specular_weight * (k_S * f_ab.x + f_ab.y);

    // Multiple scattering, from Fdez-Aguera
    float Ems = (1.0 - (f_ab.x + f_ab.y));
    float3 F_avg = specular_weight * (f0 + (1.0 - f0) / 21.0);
    float3 FmsEms = Ems * FssEss * F_avg / (1.0 - F_avg * Ems);

    return FssEss + FmsEms;
}

float3 get_ibl_radiance_ggx(ImageBasedLightingData ibl, float3 n, float3 v, float roughness)
{
    float NdotV = clamped_dot(n, v);
    float lod = roughness * float(ibl.mip_count - 1);
    float3 reflection = normalize(reflect(-v, n));
    float4 specularSample = get_specular_sample(ibl, reflection, lod);

    float3 specularLight = specularSample.rgb;

    return specularLight; 
}

public float3 get_ibl_radiance_charlie(ImageBasedLightingData ibl, float3 n, float3 v, float sheen_roughness, float3 sheen_color)
{
    float NdotV = clamped_dot(n, v);
    float lod = sheen_roughness * float(ibl.mip_count - 1);
    float3 reflection = normalize(reflect(-v, n));
    
    float2 brdfSamplePoint = clamp(float2(NdotV, sheen_roughness), float2(0.0, 0.0), float2(1.0, 1.0));
    float brdf = ibl.charlie_lookup_table.Sample(brdfSamplePoint).b;
    
    float4 sheenSample = get_sheen_sample(ibl, reflection, lod);
    
    return sheenSample.rgb * sheen_color * brdf;
}

#ifdef MATERIAL_TRANSMISSION
float3 get_transmission_sample(TransmissionTexture transmission_tex, float2 frag_coords, float roughness, float ior)
{
    float framebufferLod = log2(float(transmission_tex.framebuffer_size.x)) * apply_ior_to_roughness(roughness, ior);
    float3 transmittedLight = transmission_tex.framebuffer_sampler.SampleLevel(frag_coords.xy, framebufferLod).rgb;

    return transmittedLight;
}
#endif

#ifdef MATERIAL_TRANSMISSION
float3 get_ibl_volume_refraction(TransmissionTexture transmission_tex, float3 n, float3 v, float perceptualRoughness, float3 baseColor, float3 position, float4x4 modelMatrix, float4x4 viewMatrix, float4x4 projMatrix, float ior, float thickness, float3 attenuationColor, float attenuationDistance, float dispersion)
{
#ifdef MATERIAL_DISPERSION
    // Dispersion will spread out the ior values for each r,g,b channel
    float halfSpread = (ior - 1.0) * 0.025 * dispersion;
    float3 iors = float3(ior - halfSpread, ior, ior + halfSpread);

    float3 transmittedLight;
    float transmissionRayLength;
    for (int i = 0; i < 3; i++)
    {
        float3 transmissionRay = get_volume_transmission_ray(n, v, thickness, iors[i], modelMatrix);
        // TODO: taking length of blue ray, ideally we would take the length of the green ray. For now overwriting seems ok
        transmissionRayLength = length(transmissionRay);
        float3 refractedRayExit = position + transmissionRay;

        // Project refracted vector on the framebuffer, while mapping to normalized device coordinates.
        float4 ndcPos = mul(mul(projMatrix, viewMatrix), float4(refractedRayExit, 1.0));
        float2 refractionCoords = ndcPos.xy / ndcPos.w;
        refractionCoords += 1.0;
        refractionCoords /= 2.0;

        // Sample framebuffer to get pixel the refracted ray hits for this color channel.
        transmittedLight[i] = get_transmission_sample(transmission_tex, refractionCoords, perceptualRoughness, iors[i])[i];
    }
#else
    float3 transmissionRay = get_volume_transmission_ray(n, v, thickness, ior, modelMatrix);
    float transmissionRayLength = length(transmissionRay);
    float3 refractedRayExit = position + transmissionRay;

    // Project refracted vector on the framebuffer, while mapping to normalized device coordinates.
    float4 ndcPos = mul(mul(projMatrix, viewMatrix), float4(refractedRayExit, 1.0));
    float2 refractionCoords = ndcPos.xy / ndcPos.w;
    refractionCoords += 1.0;
    refractionCoords /= 2.0;

    // Sample framebuffer to get pixel the refracted ray hits.
    float3 transmittedLight = get_transmission_sample(transmission_tex, refractionCoords, perceptualRoughness, ior);

#endif // MATERIAL_DISPERSION
    float3 attenuatedColor = apply_volume_attenuation(transmittedLight, transmissionRayLength, attenuationColor, attenuationDistance);

    return attenuatedColor * baseColor;
}
#endif

#ifdef MATERIAL_ANISOTROPY
float3 get_ibl_radiance_anisotropy(ImageBasedLightingData ibl, float3 n, float3 v, float roughness, float anisotropy, float3 anisotropyDirection)
{
    float NdotV = clamped_dot(n, v);

    float tangentRoughness = lerp(roughness, 1.0, anisotropy * anisotropy);
    float3  anisotropicTangent  = cross(anisotropyDirection, v);
    float3  anisotropicNormal   = cross(anisotropicTangent, anisotropyDirection);
    float bendFactor          = 1.0 - anisotropy * (1.0 - roughness);
    float bendFactorPow4      = bendFactor * bendFactor * bendFactor * bendFactor;
    float3  bentNormal          = normalize(lerp(anisotropicNormal, n, bendFactorPow4));

    float lod = roughness * float(ibl.mip_count - 1);
    float3 reflection = normalize(reflect(-v, bentNormal));

    float4 specularSample = get_specular_sample(ibl, reflection, lod);

    float3 specularLight = specularSample.rgb;

    return specularLight;
}
#endif