module lights;

import functions;
import brdf;

// KHR_lights_punctual extension.
// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual

public enum LightType
{
    Directional = 0,
    Point = 1,
    Spot = 2
}

public struct Light
{
    public float3 direction;
    public float range;

    public float3 color;
    public float intensity;

    public float3 position;
    public float inner_cone_cos;

    public float outer_cone_cos;
    public LightType type;
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float get_range_attenuation(float range, float distance)
{
    if (range <= 0.0)
    {
        // negative range means unlimited
        return 1.0 / pow(distance, 2.0);
    }

    return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float get_spot_attenuation(float3 point_to_light, float3 spot_direction, float outer_cone_cos, float inner_cone_cos)
{
    float actualCos = dot(normalize(spot_direction), normalize(-point_to_light));
    if (actualCos > outer_cone_cos)
    {
        if (actualCos < inner_cone_cos)
        {
            float angularAttenuation = (actualCos - outer_cone_cos) / (inner_cone_cos - outer_cone_cos);
            return angularAttenuation * angularAttenuation;
        }
        return 1.0;
    }
    return 0.0;
}

public float3 get_light_intensity(Light light, float3 point_to_light)
{
    float range_attenuation = 1.0;
    float spot_attenuation = 1.0;

     if (light.type != LightType::Directional)
    {
        range_attenuation = get_range_attenuation(light.range, length(point_to_light));
    }

    if (light.type == LightType::Spot)
    {
        spot_attenuation = get_spot_attenuation(point_to_light, light.direction, light.outer_cone_cos, light.inner_cone_cos);
    }

    return range_attenuation * spot_attenuation * light.intensity * light.color;
}

public float3 get_punctual_radiance_transmission(float3 normal, float3 view, float3 point_to_light, float alpha_roughness, float3 base_color, float ior)
{
    float transmission_roughness = apply_ior_to_roughness(alpha_roughness, ior);

    float3 n = normalize(normal);           // Outward direction of surface point
    float3 v = normalize(view);             // Direction from surface point to view
    float3 l = normalize(point_to_light);
    float3 l_mirror = normalize(l + 2.0 * n * dot(-l, n));     // Mirror light reflection vector on surface
    float3 h = normalize(l_mirror + v);            // Halfway vector between transmission light vector and v

    float D = D_GGX(clamp(dot(n, h), 0.0, 1.0), transmission_roughness);
    float Vis = V_GGX(clamp(dot(n, l_mirror), 0.0, 1.0), clamp(dot(n, v), 0.0, 1.0), transmission_roughness);

    // Transmission BTDF
    return base_color * D * Vis;
}

public float3 get_punctual_radiance_clearcoat(float3 clearcoat_normal, float3 v, float3 l, float3 h, float v_dot_h, float3 f0, float3 f90, float clearcoat_roughness)
{
    float n_dot_l = clamped_dot(clearcoat_normal, l);
    float n_dot_v = clamped_dot(clearcoat_normal, v);
    float n_dot_h = clamped_dot(clearcoat_normal, h);

    return n_dot_l * BRDF_specular_GGX(clearcoat_roughness * clearcoat_roughness, n_dot_l, n_dot_v, n_dot_h);
}

public float3 get_punctual_radiance_sheen(float3 sheen_color, float sheen_roughness, float n_dot_l, float n_dot_v, float n_dot_h)
{
    return n_dot_l * BRDF_specular_sheen(sheen_color, sheen_roughness, n_dot_l, n_dot_v, n_dot_h);
}

// Compute attenuated light as it travels through a volume.
public float3 apply_volume_attenuation(float3 radiance, float transmission_distance, float3 attenuation_color, float attenuation_distance)
{
    if (attenuation_distance == 0)
    {
        // Attenuation distance is +∞ (which we indicate by zero), i.e. the transmitted color is not attenuated at all.
        return radiance;
    }
    else
    {
            // Compute light attenuation using Beer's law.
        float3 transmittance = pow(attenuation_color, float3(transmission_distance / attenuation_distance));
        return radiance * transmittance;
    }
}

public float3 get_volume_transmission_ray(float3 n, float3 v, float thickness, float ior, float4x4 model_matrix)
{
    // Direction of refracted light.
    float3 refraction_vector = refract(-v, normalize(n), 1.0 / ior);

    // Compute rotation-independant scaling of the model matrix.
    float3 model_scale;
    model_scale.x = length(float3(model_matrix[0].xyz));
    model_scale.y = length(float3(model_matrix[1].xyz));
    model_scale.z = length(float3(model_matrix[2].xyz));

    // The thickness is specified in local space.
    return normalize(refraction_vector) * thickness * model_scale;
}