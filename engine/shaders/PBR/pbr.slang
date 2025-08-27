#define DEBUG_NORMAL_SHADING 0
#define DEBUG_NORMAL_TEXTURE 1
#define DEBUG_NORMAL_GEOMETRY 2
#define DEBUG_TANGENT 3
#define DEBUG_BITANGENT 4
#define DEBUG_ALPHA 5
#define DEBUG_UV_0 6
#define DEBUG_UV_1 7
#define DEBUG_OCCLUSION 8
#define DEBUG_EMISSIVE 9
#define DEBUG_BASE_COLOR 10
#define DEBUG_ROUGHNESS 11
#define DEBUG_METALLIC 12
#define DEBUG_CLEARCOAT_FACTOR 13
#define DEBUG_CLEARCOAT_ROUGHNESS 14
#define DEBUG_CLEARCOAT_NORMAL 15
#define DEBUG_SHEEN_COLOR 16
#define DEBUG_SHEEN_ROUGHNESS 17
#define DEBUG_SPECULAR_FACTOR 18
#define DEBUG_SPECULAR_COLOR 19
#define DEBUG_TRANSMISSION_FACTOR 20
#define DEBUG_VOLUME_THICKNESS 21
#define DEBUG_DIFFUSE_TRANSMISSION_FACTOR 22
#define DEBUG_DIFFUSE_TRANSMISSION_COLOR_FACTOR 23
#define DEBUG_IRIDESCENCE_FACTOR 24
#define DEBUG_IRIDESCENCE_THICKNESS 25
#define DEBUG_ANISOTROPIC_STRENGTH 26
#define DEBUG_ANISOTROPIC_DIRECTION 27
#define DEBUG_NONE 100
#define DEBUG DEBUG_NONE

#define ALPHAMODE_OPAQUE 0
#define ALPHAMODE_MASK 1
#define ALPHAMODE_BLEND 2
#define ALPHAMODE ALPHAMODE_MASK

import tonemapping;
import textures;
import functions;
import brdf;
import lights;
import ibl;
import material_info;
import animation;

#ifdef MATERIAL_IRIDESCENCE
import iridescence;
#endif

ParameterBlock<SceneData> scene_data;

#ifdef USE_PUNCTUAL
ParameterBlock<Light> lights[LIGHT_COUNT + 1];
#endif

ParameterBlock<ImageBasedLightingData> ibl_data;

ParameterBlock<MaterialInputs> material_inputs;

ParameterBlock<ScaledTexture> normal_texture;
ParameterBlock<FactoredTexture> emissive_texture;
ParameterBlock<ScaledTexture> occlusion_texture;

#ifdef MATERIAL_METALLICROUGHNESS
ParameterBlock<MaterialTexture> color_texture;
ParameterBlock<MaterialTexture> metallic_roughness_texture;
#endif 

#ifdef MATERIAL_SPECULARGLOSSINESS
ParameterBlock<MaterialTexture> specular_glossiness_texture;
ParameterBlock<MaterialTexture> diffuse_texture;
#endif

#ifdef MATERIAL_CLEARCOAT
ParameterBlock<MaterialTexture> clearcoat_texture;
ParameterBlock<MaterialTexture> clearcoat_roughness_texture;
ParameterBlock<MaterialTexture> clearcoat_normal_texture;
#endif

#ifdef MATERIAL_SHEEN
ParameterBlock<MaterialTexture> sheen_texture;
ParameterBlock<MaterialTexture> sheen_roughness_texture;
#endif

#ifdef MATERIAL_SPECULAR
ParameterBlock<MaterialTexture> specular_texture;
ParameterBlock<MaterialTexture> specular_color_texture;
#endif

#ifdef MATERIAL_TRANSMISSION
ParameterBlock<TransmissionTexture> transmission_texture;
#endif

#ifdef MATERIAL_VOLUME
ParameterBlock<MaterialTexture> thickness_texture;
#endif

#ifdef MATERIAL_IRIDESCENCE
ParameterBlock<MaterialTexture> iridescence_texture;
ParameterBlock<MaterialTexture> iridescence_thickness_texture;
#endif

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
ParameterBlock<MaterialTexture> diffuse_transmission_texture;
ParameterBlock<MaterialTexture> diffuse_transmission_color_texture;
#endif

#ifdef MATERIAL_ANISOTROPY
ParameterBlock<MaterialTexture> anisotropy_texture;
#endif

#ifdef USE_INSTANCING
ParameterBlock<float4x4> instance_model_matrix;
#endif

#ifdef HAS_VERT_NORMAL_UV_TRANSFORM
ParameterBlock<float3x3> normal_uv_transform;
#endif

ParameterBlock<AnimationData> animation_data;

struct VertexOutput
{
    float4 position: SV_Position;
    Vertex vertex;
    float2 uv0;
    float2 uv1;
};

float4 get_position(uint vertex_id, Vertex v)
{
    float4 pos = float4(v.position, 1);

#ifdef USE_MORPHING
    pos += get_target_position(animation_data, vertex_id);
#endif

#ifdef USE_SKINNING
    pos = mul(get_skinning_matrix(animation_data), pos);
#endif

    return pos;
}

#ifdef HAS_NORMAL_VEC3
float3 get_normal(uint vertex_id, Vertex v)
{
    float3 normal = v.normal;

#ifdef USE_MORPHING
    normal += get_target_normal(animation_data, vertex_id);
#endif

#ifdef USE_SKINNING
    normal = mul(float3x3(get_skinning_matrix(animation_data)), normal);
#endif

    return normalize(normal);
}
#endif

#ifdef HAS_NORMAL_VEC3
#ifdef HAS_TANGENT_VEC4
float3 get_tangent(uint vertex_id, Vertex v)
{
    float3 tangent = v.tangent.xyz;

#ifdef USE_MORPHING
    tangent += get_target_tangent(animation_data, vertex_id);
#endif

#ifdef USE_SKINNING
    tangent = mul(float3x3(get_skinning_matrix(animation_data)), tangent);
#endif

    return normalize(tangent);
}
#endif
#endif


[shader("vertex")]
VertexOutput vert_main(uint vertex_id : SV_VertexID, [[push_constant]] uniform Vertex* vertex_buffer, [[push_constant]] uniform ModelInfo model_info)
{
    Vertex v = vertex_buffer[vertex_id];

#ifdef USE_INSTANCING
    float4x4 model_matrix = instance_model_matrix;
    float4x4 normal_matrix = transpose(inverse(model_matrix));
#else
    float4x4 model_matrix = model_info.model_matrix;
    float4x4 normal_matrix = model_info.normal_matrix;
#endif

#ifdef USE_SKINNING
    float4 pos = get_position(vertex_id, v);
#else
    float4 pos = mul(model_matrix, get_position(vertex_id, v));
#endif

    Vertex out_vertex;
    VertexOutput out;
    out.vertex = out_vertex;
    out.vertex.position = pos.xyz / pos.w;

#ifdef HAS_NORMAL_VEC3
#ifdef HAS_TANGENT_VEC4
    float3 tangent = get_tangent(vertex_id, v);
    float3 normal_w = normalize(float3(mul(normal_matrix, float4(get_normal(vertex_id, v), 0.0))));
    float3 tangent_w = float3(mul(model_matrix, float4(tangent, 0)));
    float3 bitangent_w = cross(normal_w, tangent_w) * v.tangent.w;

#ifdef HAS_VERT_NORMAL_UV_TRANSFORM
    tangent_w = mul(normal_uv_transform, tangent_w);
    bitangent_w = mul(normal_uv_transform, bitangent_w);
#endif

    bitangent_w = normalize(bitangent_w);
    tangent_w = normalize(tangent_w);
    out.vertex.tbn = float3x3(tangent_w, bitangent_w, normal_w);
#else
    out.vertex.normal = normalize(float3(mul(normal_matrix, float4(get_normal(vertex_id, v), 0.0))));
#endif
#endif

    out.uv0 = float2(0.0, 0.0);
    out.uv1 = float2(0.0, 0.0);

#ifdef HAS_TEXCOORD_0_VEC2
    out.uv0 = v.uv0;
#endif

#ifdef HAS_TEXCOORD_1_VEC2
    out.uv1 = v.uv1;
#endif

#ifdef USE_MORPHING
    out.uv0 += get_target_tex_coord0(animation_data, vertex_id);
    out.uv1 += get_target_tex_coord1(animation_data, vertex_id);
#endif

#ifdef HAS_COLOR_0_VEC3
    out.color = v.color;
#ifdef USE_MORPHING
    out.color = clamp(out.color + get_target_color0(animation_data, vertex_id).xyz, 0, 1.f);
#endif
#endif

#ifdef HAS_COLOR_0_VEC4
    out.color = v.color;
#ifdef USE_MORPHING
    out.color = clamp(out.color + get_target_color0(animation_data, vertex_id).xyz, 0, 1.f);
#endif
#endif

    out.position = mul(model_info.model_matrix, pos);

    return out;
}

[shader("fragment")]
float4 frag_main(VertexOutput in, [[push_constant]] uniform ModelInfo model_info) : SV_Target
{
    float4 base_color = get_base_color(material_inputs, in.uv0, in.uv1);

#if ALPHAMODE == ALPHAMODE_OPAQUE
    base_color.a = 1.0;
#endif

    float3 color = float3(0);

    float3 v = normalize(scene_data.camera - in.position.xyz);
    NormalInfo normal_info = get_normal_info(in.vertex, normal_texture, normal_texture.scale, v, in.uv0, in.uv1);
    float3 n = normal_info.normal;
    float3 t = normal_info.tangent;
    float3 b = normal_info.bitangent;

    float n_dot_v = clamped_dot(n, v);
    float t_dot_v = clamped_dot(t, v);
    float b_dot_v = clamped_dot(b, v);

    Material material;
    material.base_color = base_color.rgb;

    // The default index of refraction of 1.5 yields a dielectric normal incidence reflectance of 0.04.
    material.ior = 1.5;
    material.f0_dielectric = float3(0.04);
    material.specular_weight = 1.0;

    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    material.f90 = float3(1.0);
    material.f90_dielectric = material.f90;

#ifdef MATERIAL_IOR
    material = get_ior_info(material_inputs, material);
#endif

#ifdef MATERIAL_METALLICROUGHNESS
    material = get_metallic_roughness_info(material_inputs, material, in.uv0, in.uv1);
#endif

#ifdef MATERIAL_SHEEN
    material = get_sheen_info(material_inputs, material, in.uv0, in.uv1);
#endif

#ifdef MATERIAL_CLEARCOAT
    material = get_clearcoat_info(material_inputs, material, normal_info, in.uv0, in.uv1);
#endif

#ifdef MATERIAL_SPECULAR
    material = get_specular_info(material_inputs, material, in.uv0, in.uv1);
#endif

#ifdef MATERIAL_TRANSMISSION
    material = get_transmission_info(material_inputs, material, in.uv0, in.uv1);
#endif

#ifdef MATERIAL_VOLUME
    material = get_volume_info(material_inputs, material, in.uv0, in.uv1);
#endif

#ifdef MATERIAL_IRIDESCENCE
    material = get_iridescence_info(material_inputs, material, in.uv0, in.uv1);
#endif

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
    material = get_diffuse_transmission_info(material_inputs, material, in.uv0, in.uv1);
#endif

#ifdef MATERIAL_ANISOTROPY
    material = get_anisotropy_info(material_inputs, material, normal_info, in.uv0, in.uv1);
#endif

    material.perceptual_roughness = clamp(material.perceptual_roughness, 0.0, 1.0);
    material.metallic = clamp(material.metallic, 0.0, 1.0);

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness.
    material.alpha_roughness = material.perceptual_roughness * material.perceptual_roughness;

     // LIGHTING
    float3 f_specular_dielectric = float3(0.0);
    float3 f_specular_metal = float3(0.0);
    float3 f_diffuse = float3(0.0);
    float3 f_dielectric_brdf_ibl = float3(0.0);
    float3 f_metal_brdf_ibl = float3(0.0);
    float3 f_emissive = float3(0.0);
    float3 clearcoat_brdf = float3(0.0);
    float3 f_sheen = float3(0.0);
    float3 f_specular_transmission = float3(0.0);
    float3 f_diffuse_transmission = float3(0.0);

    float clearcoat_factor = 0;
    float3 clearcoat_fresnel = float3(0.0);

    float albedo_sheen_scaling = 1.f;
    float diffuse_transmission_thickness = 1.0;

#ifdef MATERIAL_IRIDESCENCE
    float3 iridescenceFresnel_dielectric = eval_iridescence(1.0, material.iridescence_ior, n_dot_v, material.iridescence_thickness, material.f0_dielectric);
    float3 iridescenceFresnel_metallic = eval_iridescence(1.0, material.iridescence_ior, n_dot_v, material.iridescence_thickness, base_color.rgb);

    if (material.iridescence_thickness == 0.0) {
        material.iridescence_factor = 0.0;
    }
#endif

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
#ifdef MATERIAL_VOLUME
    diffuse_transmission_thickness = material.thickness *
        (length(float3(model_info.model_matrix[0].xyz)) + length(float3(model_info.model_matrix[1].xyz)) + length(float3(model_info.model_matrix[2].xyz))) / 3.0;
#endif
#endif

#ifdef MATERIAL_CLEARCOAT
    clearcoat_factor = material.clearcoat_factor;
    clearcoat_fresnel = f_schlick(material.clearcoat_f0, material.clearcoat_f90, clamped_dot(material.clearcoat_normal, v));
#endif

    // Calculate lighting contribution from image based lighting source (IBL)
#if defined(USE_IBL) || defined(MATERIAL_TRANSMISSION)
    f_diffuse = get_diffuse_light(ibl_data, n) * base_color.rgb ;

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
    float3 diffuseTransmissionIBL = get_diffuse_light(ibl_data, -n) * material.diffuse_transmission_color_factor;
#ifdef MATERIAL_VOLUME
        diffuseTransmissionIBL = apply_volume_attenuation(diffuseTransmissionIBL, diffuse_transmission_thickness, material.attenuation_color, material.attenuation_distance);
#endif
    f_diffuse = lerp(f_diffuse, diffuseTransmissionIBL, material.diffuse_transmission_factor);
#endif


#if defined(MATERIAL_TRANSMISSION)
    f_specular_transmission = get_ibl_volume_refraction(
        transmission_texture, n, v,
        material.perceptual_roughness,
        base_color.rgb, in.position, model_info.model_matrix, scene_data.view_matrix, scene_data.projection_matrix,
        material.ior, material.thickness, material.attenuation_color, material.attenuation_distance, material.dispersion);
    f_diffuse = lerp(f_diffuse, f_specular_transmission, material.transmission_factor);
#endif

#ifdef MATERIAL_ANISOTROPY
    f_specular_metal = get_ibl_radiance_anisotropy(ibl_data, n, v, material.perceptual_roughness, material.anisotropy_strength, material.anisotropic_b);
    f_specular_dielectric = f_specular_metal;
#else
    f_specular_metal = get_ibl_radiance_ggx(ibl_data, n, v, material.perceptual_roughness);
    f_specular_dielectric = f_specular_metal;
#endif


    // Calculate fresnel mix for IBL  

    float3 f_metal_fresnel_ibl = get_ibl_ggx_fresnel(ibl_data, n, v, material.perceptual_roughness, base_color.rgb, 1.0);
    f_metal_brdf_ibl = f_metal_fresnel_ibl * f_specular_metal;

    float3 f_dielectric_fresnel_ibl = get_ibl_ggx_fresnel(ibl_data, n, v, material.perceptual_roughness, material.f0_dielectric, material.specular_weight);
    f_dielectric_brdf_ibl = lerp(f_diffuse, f_specular_dielectric,  f_dielectric_fresnel_ibl);

#ifdef MATERIAL_IRIDESCENCE
    f_metal_brdf_ibl = lerp(f_metal_brdf_ibl, f_specular_metal * iridescenceFresnel_metallic, material.iridescence_factor);
    f_dielectric_brdf_ibl = lerp(f_dielectric_brdf_ibl, rgb_mix(f_diffuse, f_specular_dielectric, iridescenceFresnel_dielectric), material.iridescence_factor);
#endif


#ifdef MATERIAL_CLEARCOAT
    clearcoat_brdf = get_ibl_radiance_ggx(ibl_data, material.clearcoat_normal, v, material.clearcoat_roughness);
#endif

#ifdef MATERIAL_SHEEN
    f_sheen = get_ibl_radiance_charlie(ibl_data, n, v, material.sheen_roughness, material.sheen_color);
    albedo_sheen_scaling = 1.0 - lerp(material.sheen_color, albedo_sheen_scaling_lut(ibl_data, n_dot_v, material.sheen_roughness));
#endif

    color = lerp(f_dielectric_brdf_ibl, f_metal_brdf_ibl, material.metallic);
    color = f_sheen + color * albedo_sheen_scaling;
    color = lerp(color, clearcoat_brdf, clearcoat_factor * clearcoat_fresnel);

#ifdef HAS_OCCLUSION_MAP
    float ao = 1.0;
    ao = occlusion_texture.Sample(get_occlusion_uv(in.uv0, in.uv1)).r;
    color = color * (1.0 + occlusion_texture.scale * (ao - 1.0)); 
#endif

#endif //end USE_IBL


    f_diffuse = float3(0.0);
    f_specular_dielectric = float3(0.0);
    f_specular_metal = float3(0.0);
    float3 f_dielectric_brdf = float3(0.0);
    float3 f_metal_brdf = float3(0.0);

#ifdef USE_PUNCTUAL
    for(int i = 0; i < LIGHT_COUNT; i++)
    {
        Light light = lights[i];

        float3 point_to_light;
        if (light.type != LightType::Directional)
        {
            point_to_light = light.position - in.position;
        }
        else
        {
            point_to_light = -light.direction;
        }

        // BSTF
        float3 l = normalize(point_to_light);   // Direction from surface point to light
        float3 h = normalize(l + v);          // Direction of the vector between l and v, called halfway vector
        float NdotL = clamped_dot(n, l);
        float NdotV = clamped_dot(n, v);
        float NdotH = clamped_dot(n, h);
        float LdotH = clamped_dot(l, h);
        float VdotH = clamped_dot(v, h);

        float3 dielectric_fresnel = f_schlick(material.f0_dielectric * material.specular_weight, material.f90_dielectric, abs(VdotH));
        float3 metal_fresnel = f_schlick(base_color.rgb, float3(1.0), abs(VdotH));
        
        float3 lightIntensity = get_light_intensity(light, point_to_light);
        
        float3 l_diffuse = lightIntensity * NdotL * BRDF_lambertian(base_color.rgb);
        float3 l_specular_dielectric = float3(0.0);
        float3 l_specular_metal = float3(0.0);
        float3 l_dielectric_brdf = float3(0.0);
        float3 l_metal_brdf = float3(0.0);
        float3 l_clearcoat_brdf = float3(0.0);
        float3 l_sheen = float3(0.0);
        float l_albedoSheenScaling = 1.0;

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
        l_diffuse = l_diffuse * (1.0 - material.diffuse_transmission_factor);
        if (dot(n, l) < 0.0) {
            float diffuseNdotL = clamped_dot(-n, l);
            float3 diffuse_btdf = lightIntensity * diffuseNdotL * BRDF_lambertian(material.diffuse_transmission_color_factor);

            float3 l_mirror = normalize(l + 2.0 * n * dot(-l, n)); // Mirror light reflection vector on surface
            float diffuseVdotH = clamped_dot(v, normalize(l_mirror + v));
            dielectric_fresnel = f_schlick(material.f0_dielectric * material.specular_weight, material.f90_dielectric, abs(diffuseVdotH));

#ifdef MATERIAL_VOLUME
            diffuse_btdf = apply_volume_attenuation(diffuse_btdf, diffuse_transmission_thickness, material.attenuation_color, material.attenuation_distance);
#endif
            l_diffuse += diffuse_btdf * material.diffuse_transmission_factor;
        }
#endif // MATERIAL_DIFFUSE_TRANSMISSION

        // BTDF (Bidirectional Transmittance Distribution Function)
#ifdef MATERIAL_TRANSMISSION
        // If the light ray travels through the geometry, use the point it exits the geometry again.
        // That will change the angle to the light source, if the material refracts the light ray.
        float3 transmissionRay = get_volume_transmission_ray(n, v, material.thickness, material.ior, model_info.model_matrix);
        point_to_light -= transmissionRay;
        l = normalize(point_to_light);

        float3 transmittedLight = lightIntensity * get_punctual_radiance_transmission(n, v, l, material.alpha_roughness, base_color.rgb, material.ior);

#ifdef MATERIAL_VOLUME
        transmittedLight = apply_volume_attenuation(transmittedLight, length(transmissionRay), material.attenuation_color, material.attenuation_distance);
#endif
        l_diffuse = lerp(l_diffuse, transmittedLight, material.transmission_factor);
#endif

        // Calculation of analytical light
        // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
        float3 intensity = get_light_intensity(light, point_to_light);

#ifdef MATERIAL_ANISOTROPY
        l_specular_metal = intensity * NdotL * BRDF_specularGGXAnisotropy(material.alpha_roughness, material.anisotropy_strength, n, v, l, h, material.anisotropic_t, material.anisotropic_b);
        l_specular_dielectric = l_specular_metal;
#else
        l_specular_metal = intensity * NdotL * BRDF_specular_GGX(material.alpha_roughness, NdotL, NdotV, NdotH);
        l_specular_dielectric = l_specular_metal;
#endif

        l_metal_brdf = metal_fresnel * l_specular_metal;
        l_dielectric_brdf = lerp(l_diffuse, l_specular_dielectric, dielectric_fresnel); // Do we need to handle vec3 fresnel here?

        
#ifdef MATERIAL_IRIDESCENCE
        l_metal_brdf = lerp(l_metal_brdf, l_specular_metal * iridescenceFresnel_metallic, material.iridescence_factor);
        l_dielectric_brdf = lerp(l_dielectric_brdf, rgb_mix(l_diffuse, l_specular_dielectric, iridescenceFresnel_dielectric), material.iridescence_factor);
#endif


#ifdef MATERIAL_CLEARCOAT
        l_clearcoat_brdf = intensity * get_punctual_radiance_clearcoat(material.clearcoat_normal, v, l, h, VdotH,
            material.clearcoat_f0, material.clearcoat_f90, material.clearcoat_roughness);
#endif

#ifdef MATERIAL_SHEEN
        l_sheen = intensity * get_punctual_radiance_sheen(material.sheen_color_factor, material.sheen_roughness_factor, NdotL, NdotV, NdotH);
        l_albedoSheenScaling = min(1.0 - max3(material.sheen_color_factor) * albedo_sheen_scaling_lut(NdotV, material.sheen_roughness_factor),
            1.0 - max3(material.sheen_color_factor) * albedo_sheen_scaling_lut(NdotL, material.sheen_roughness_factor));
#endif
        
        float3 l_color = lerp(l_dielectric_brdf, l_metal_brdf, material.metallic);
        l_color = l_sheen + l_color * l_albedoSheenScaling;
        l_color = lerp(l_color, l_clearcoat_brdf, clearcoat_factor * clearcoat_fresnel);
        color += l_color;
    }
#endif

    f_emissive = emissive_texture.factor;
#ifdef MATERIAL_EMISSIVE_STRENGTH
    f_emissive *= material_inputs.emissive_strength;
#endif
#ifdef HAS_EMISSIVE_MAP
    f_emissive *= emissive_texture.Sample(get_emissive_uv(in.uv0, in.uv1)).rgb;
#endif

#ifdef MATERIAL_UNLIT
    color = base_color.rgb;
    color = f_emissive * (1.0 - clearcoat_factor * clearcoat_fresnel) + color;
#endif

float4 output_color;

#if DEBUG == DEBUG_NONE

#if ALPHAMODE == ALPHAMODE_MASK
    // Late discard to avoid sampling artifacts. See https://github.com/KhronosGroup/glTF-Sample-Viewer/issues/267
    if (base_color.a < material_inputs.alpha_cutoff)
    {
        discard;
    }
    base_color.a = 1.0;
#endif

#ifdef LINEAR_OUTPUT
    output_color = float4(color.rgb, base_color.a);
#else
    output_color = float4(tonemap(scene_data.exposure, color), base_color.a);
#endif

#else
    // In case of missing data for a debug view, render a checkerboard.
    output_color = float4(1.0);
    {
        float frequency = 0.02;
        float gray = 0.9;

        float2 v1 = step(0.5, fract(frequency * in.position.xy));
        float2 v2 = step(0.5, float2(1.0) - fract(frequency * in.position.xy));
        output_color.rgb *= gray + v1.x * v1.y + v2.x * v2.y;
    }
#endif


// Debug views:

// Generic:
#if DEBUG == DEBUG_UV_0 && defined(HAS_TEXCOORD_0_VEC2)
    output_color.rgb = float3(in.uv0, 0);
#endif
#if DEBUG == DEBUG_UV_1 && defined(HAS_TEXCOORD_1_VEC2)
    output_color.rgb = float3(in.uv1, 0);
#endif
#if DEBUG == DEBUG_NORMAL_TEXTURE && defined(HAS_NORMAL_MAP)
    output_color.rgb = (normal_info.normal_tex + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_NORMAL_SHADING
    output_color.rgb = (n + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_NORMAL_GEOMETRY
    output_color.rgb = (normal_info.geometry_normal + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_TANGENT
    output_color.rgb = (normal_info.tangent + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_BITANGENT
    output_color.rgb = (normal_info.bitangent + 1.0) / 2.0;
#endif
#if DEBUG == DEBUG_ALPHA
    output_color.rgb = float3(base_color.a);
#endif
#if DEBUG == DEBUG_OCCLUSION && defined(HAS_OCCLUSION_MAP)
    output_color.rgb = float3(ao);
#endif
#if DEBUG == DEBUG_EMISSIVE
    output_color.rgb = linear_to_sRGB(f_emissive);
#endif

#if DEBUG == DEBUG_METALLIC
    output_color.rgb = float3(material.metallic);
#endif
#if DEBUG == DEBUG_ROUGHNESS
    output_color.rgb = float3(material.perceptual_roughness);
#endif
#if DEBUG == DEBUG_BASE_COLOR
    output_color.rgb = linear_to_sRGB(material.base_color);
#endif

// Clearcoat:
#ifdef MATERIAL_CLEARCOAT
#if DEBUG == DEBUG_CLEARCOAT_FACTOR
    output_color.rgb = float3(material.clearcoat_factor);
#endif
#if DEBUG == DEBUG_CLEARCOAT_ROUGHNESS
    output_color.rgb = float3(material.clearcoat_roughness);
#endif
#if DEBUG == DEBUG_CLEARCOAT_NORMAL
    output_color.rgb = (material.clearcoat_normal + float3(1)) / 2.0;
#endif
#endif

// Sheen:
#ifdef MATERIAL_SHEEN
#if DEBUG == DEBUG_SHEEN_COLOR
    output_color.rgb = material.sheen_color_factor;
#endif
#if DEBUG == DEBUG_SHEEN_ROUGHNESS
    output_color.rgb = float3(material.sheen_roughness_factor);
#endif
#endif

// Specular:
#ifdef MATERIAL_SPECULAR
#if DEBUG == DEBUG_SPECULAR_FACTOR
    output_color.rgb = float3(material.specular_weight);
#endif

#if DEBUG == DEBUG_SPECULAR_COLOR
float3 specular_texture_sample = float3(1.0);
#ifdef HAS_SPECULAR_COLOR_MAP
    specular_texture_sample.rgb = specular_color_texture.Sample(get_specular_color_uv(in.uv0, in.uv1)).rgb;
#endif
    output_color.rgb = material_inputs.specular_color_factor * specular_texture_sample.rgb;
#endif
#endif

// Transmission, Volume:
#ifdef MATERIAL_TRANSMISSION
#if DEBUG == DEBUG_TRANSMISSION_FACTOR
    output_color.rgb = float3(material.transmission_factor);
#endif
#endif
#ifdef MATERIAL_VOLUME
#if DEBUG == DEBUG_VOLUME_THICKNESS
    output_color.rgb = float3(material.thickness / material_inputs.thickness_factor);
#endif
#endif

// Iridescence:
#ifdef MATERIAL_IRIDESCENCE
#if DEBUG == DEBUG_IRIDESCENCE_FACTOR
    output_color.rgb = float3(material.iridescence_factor);
#endif
#if DEBUG == DEBUG_IRIDESCENCE_THICKNESS
    output_color.rgb = float3(material.iridescence_thickness / 1200.0);
#endif
#endif

// Anisotropy:
#ifdef MATERIAL_ANISOTROPY
#if DEBUG == DEBUG_ANISOTROPIC_STRENGTH
    output_color.rgb = float3(material.anisotropy_strength);
#endif
#if DEBUG == DEBUG_ANISOTROPIC_DIRECTION
    float2 direction = float2(1.0, 0.0);
#ifdef HAS_ANISOTROPY_MAP
    direction = anisotropy_texture.Sample(get_anisotropy_uv(in.uv0, in.uv1)).xy;
    direction = direction * 2.0 - float2(1.0); // [0, 1] -> [-1, 1]
#endif
    float2 direction_rotation = material_inputs.anisotropy.xy; // cos(theta), sin(theta)
    float2x2 rotation_matrix = float2x2(direction_rotation.x, direction_rotation.y, -direction_rotation.y, direction_rotation.x);
    direction = mul(rotation_matrix, direction);
    direction = (direction + float2(1.0)) * 0.5; // [-1, 1] -> [0, 1]

    output_color.rgb = float3(direction, 0.0);
#endif
#endif

// Diffuse Transmission:
#ifdef MATERIAL_DIFFUSE_TRANSMISSION
#if DEBUG == DEBUG_DIFFUSE_TRANSMISSION_FACTOR
    output_color.rgb = linear_to_sRGB(float3(material.diffuse_transmission_factor));
#endif
#if DEBUG == DEBUG_DIFFUSE_TRANSMISSION_COLOR_FACTOR
    output_color.rgb = linear_to_sRGB(material.diffuse_transmission_color_factor);
#endif
#endif

return output_color;
}