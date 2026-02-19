module portal_lights;

import portal_common;
import portal_pbr;
import portal_scene;

public static const uint MAX_POINT_LIGHTS = 128;
public static const uint MAX_SPOT_LIGHTS = 128;

//////////////////////////////////////////
// Declerations
//////////////////////////////////////////

public struct DirectionalLight
{
    public float3 direction;
    public float multiplier;
    public float3 radiance;
    public float shadow_amount;
};


public struct PointLight
{
    public float3 position;
    public float multiplier;
    public float3 radiance;
    public float min_radius;
    public float radius;
    public float falloff;
    public float light_size;
    public bool casts_shadows;
};

public struct SpotLight
{ 
    public float3 position;
    public float multiplier;
    public float3 radiance;
    public float angle_attenuation;
    public float3 direction;
    public float range;
    public float angle;
    public float falloff;
    public bool soft_shadows;
    public bool casts_shadows;
};

public struct SceneDirectionalLight
{
    public DirectionalLight directional_light;
    public float3 camera_position;
    public float enviornment_map_itensity;
};

public struct ScenePointLights
{
    public uint light_count;
    public PointLight point_lights[MAX_POINT_LIGHTS];
}

public struct SceneSpotLights
{
    public uint light_count;
    public SpotLight spot_lights[MAX_SPOT_LIGHTS];
}

public struct SceneLights
{
    public SceneDirectionalLight directional_light;
    public ScenePointLights point_lights;
    public SceneSpotLights spot_lights;
};

public struct PBRParameters
{
    public float3 albedo;
    public float metalic;
    public float roughness;

    public float3 normal;
    public float3 view;
    public float ndotv;
}

//////////////////////////////////////////
// Functions
//////////////////////////////////////////

//////////////////////////////////////////
// Directional Lights
//////////////////////////////////////////

public float3 calculate_directional_lights(float3 f0, PBRParameters params, SceneDirectionalLight light)
{
    float3 result;

    for (int i = 0; i < 1; i++) // Currently only support 1 directional light
    {
        if(light.directional_light.multiplier == 0.f)
            continue;

        float3 li = light.directional_light.direction;
        float3 l_radiance = light.directional_light.radiance * light.directional_light.multiplier;
        float3 lh = normalize(li + params.view);

        // Calculate angles between surface normal and various light vectors.
        float cos_li = max(0.f, dot(params.normal, li));
        float cos_lh = max(0.f, dot(params.normal, lh));

        float3 f = fresnel_schlick_roughness(f0, max(0.f, dot(lh, params.view)), params.roughness);
        float d = ndfggx(cos_lh, params.roughness);
        float g = ga_schlick_ggx(cos_li, params.ndotv, params.roughness);

        float3 kd = (1.f - f) * (1.f - params.metalic);
        float3 diffuse_brdf = kd * params.albedo; // TODO: should I divide by pi here?

        // Cook - Torrance
        float3 specular_brdf = (f * d * g) / max(EPSILON, 4.f * cos_li * params.ndotv);
        specular_brdf = clamp(specular_brdf, float3(0.f), float3(10.f));
        result += (diffuse_brdf + specular_brdf) * l_radiance * cos_li;
    }

    return result;
}


// TODO: implement point and spot lights