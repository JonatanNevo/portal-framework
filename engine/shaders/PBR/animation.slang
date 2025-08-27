module animation;

public struct AnimationData
{
    #ifdef HAS_MORPH_TARGETS
    public Sampler2DArray morph_targets_sampler;
    #endif

    #ifdef USE_MORPHING
    public float morph_weights[WEIGHT_COUNT];
    #endif

    #ifdef HAS_JOINTS_0_VEC4
    public float4 joints_0;
    #endif

    #ifdef HAS_JOINTS_1_VEC4
    public float4 joints_1;
    #endif

    #ifdef HAS_WEIGHTS_0_VEC4
    public float4 weights_0;
    #endif

    #ifdef HAS_WEIGHTS_1_VEC4
    public float4 weights_1;
    #endif

    #ifdef USE_SKINNING
    public Sampler2D joints_sampler;
    #endif
}

#ifdef USE_SKINNING

float4x4 get_matrix_from_texture(Sampler2D sampler, int index)
{
    float4x4 result = float4x4(1.0f);

    uint width, height;
    sampler.GetDimensions(width, height);
    int pixel_index = index * 4;

    for (int i = 0; i < 4; ++i)
    {
        int x = (pixel_index + i) % width;
         //Rounding mode of integers is undefined:
        //https://www.khronos.org/registry/OpenGL/specs/es/3.0/GLSL_ES_Specification_3.00.pdf (section 12.33)
        int y = (pixel_index + i) / width;
        result[i] = sampler.Load(int3(x, y, 0));
    }
    return result;
}

public float4x4 get_skinning_matrix(AnimationData anim_data)
{
    float4x4 skin = float4x4(0.0f);


#if defined(HAS_WEIGHTS_0_VEC4) && defined(HAS_JOINTS_0_VEC4)
     skin +=
        anim_data.weights_0.x * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_0.x) * 2) +
        anim_data.weights_0.y * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_0.y) * 2) +
        anim_data.weights_0.z * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_0.z) * 2) +
        anim_data.weights_0.w * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_0.w) * 2);
#endif

#if defined(HAS_WEIGHTS_1_VEC4) && defined(HAS_JOINTS_1_VEC4)
    skin +=
         anim_data.weights_1.x * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_1.x) * 2) +
         anim_data.weights_1.y * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_1.y) * 2) +
         anim_data.weights_1.z * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_1.z) * 2) +
         anim_data.weights_1.w * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_1.w) * 2);
#endif

    if (all(skin == float4x4(0.0f))) { 
        return float4x4(1.0f); 
    }

    return skin;
}

float4x4 get_skinning_normal_matrix(AnimationData anim_data)
{
    float4x4 skin = float4x4(0.0f);

#if defined(HAS_WEIGHTS_0_VEC4) && defined(HAS_JOINTS_0_VEC4)
    skin +=
        anim_data.weights_0.x * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_0.x) * 2 + 1) +
        anim_data.weights_0.y * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_0.y) * 2 + 1) +
        anim_data.weights_0.z * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_0.z) * 2 + 1) +
        anim_data.weights_0.w * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_0.w) * 2 + 1);
#endif

#if defined(HAS_WEIGHTS_1_VEC4) && defined(HAS_JOINTS_1_VEC4)
    skin +=
        anim_data.weights_1.x * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_1.x) * 2 + 1) +
        anim_data.weights_1.y * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_1.y) * 2 + 1) +
        anim_data.weights_1.z * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_1.z) * 2 + 1) +
        anim_data.weights_1.w * get_matrix_from_texture(anim_data.joints_sampler, int(anim_data.joints_1.w) * 2 + 1);
#endif

    if (all(skin == float4x4(0.0f))) { 
        return float4x4(1.0f); 
    }

    return skin;
}

#endif

#ifdef USE_MORPHING

#ifdef HAS_MORPH_TARGETS
public float4 get_displacment(AnimationData anim_data, int vertex_id, int target_index, int tex_size)
{
    int x = vertex_id % tex_size;
    //Rounding mode of integers is undefined:
    //https://www.khronos.org/registry/OpenGL/specs/es/3.0/GLSL_ES_Specification_3.00.pdf (section 12.33)
    int y = (vertex_id - x) / tex_size;
    return anim_data.morph_targets_sampler.Load({x, y, target_index});
}
#endif

public float4 get_target_position(AnimationData anim_data, int vertex_id)
{
    float4 position = float4(0.f);

#ifdef HAS_MORPH_TARGET_POSITION
    uint tex_size , height, depth;
    anim_data.morph_targets_sampler.GetDimensions(tex_size, height, depth);

    for(int i = 0; i < WEIGHT_COUNT; ++i)
    {
        float4 displacment = get_displacment(anim_data, vertex_id, MORPH_TARGET_POSITION_OFFSET + i, tex_size);
        position += anim_data.morph_weights[i] * displacment;
    }
#endif

    return position;
}

public float3 get_target_normal(AnimationData anim_data, int vertex_id)
{
    float3 normal = float3(0.f);

#ifdef HAS_MORPH_TARGET_NORMAL
    uint tex_size , height, depth;
    anim_data.morph_targets_sampler.GetDimensions(tex_size, height, depth);

    for(int i = 0; i < WEIGHT_COUNT; ++i)
    {
        float4 displacment = get_displacment(anim_data, vertex_id, MORPH_TARGET_NORMAL_OFFSET + i, tex_size);
        normal += anim_data.morph_weights[i] * displacment;
    }
#endif

    return normal;
}

public float2 get_target_tex_coord0(AnimationData anim_data, int vertex_id)
{
    float2 uv = float2(0.f);

#ifdef HAS_MORPH_TARGET_TEXCOORD0
    uint tex_size , height, depth;
    anim_data.morph_targets_sampler.GetDimensions(tex_size, height, depth);

    for(int i = 0; i < WEIGHT_COUNT; ++i)
    {
        float4 displacment = get_displacment(anim_data, vertex_id, MORPH_TARGET_TEXCOORD0_OFFSET + i, tex_size);
        uv += anim_data.morph_weights[i] * displacment.xy;
    }
#endif

    return uv;
}

public float2 get_target_tex_coord1(AnimationData anim_data, int vertex_id)
{
    float2 uv = float2(0.f);

#ifdef HAS_MORPH_TARGET_TEXCOORD1
    uint tex_size , height, depth;
    anim_data.morph_targets_sampler.GetDimensions(tex_size, height, depth);

    for(int i = 0; i < WEIGHT_COUNT; ++i)
    {
        float4 displacment = get_displacment(anim_data, vertex_id, MORPH_TARGET_TEXCOORD1_OFFSET + i, tex_size);
        uv += anim_data.morph_weights[i] * displacment.xy;
    }
#endif

    return uv;
}

public float3 get_target_tangent(AnimationData anim_data, int vertex_id)
{
    float3 tangent = float3(0.f);

#ifdef HAS_MORPH_TARGET_TANGENT
    uint tex_size , height, depth;
    anim_data.morph_targets_sampler.GetDimensions(tex_size, height, depth);

    for(int i = 0; i < WEIGHT_COUNT; ++i)
    {
        float4 displacment = get_displacment(anim_data, vertex_id, MORPH_TARGET_TANGENT_OFFSET + i, tex_size);
        tangent += anim_data.morph_weights[i] * displacment.xyz;
    }
#endif

    return tangent;
}

public float4 get_target_color(AnimationData anim_data, int vertex_id)
{
    float4 color = float4(0.f);

#ifdef HAS_MORPH_TARGET_COLOR
    uint tex_size , height, depth;
    anim_data.morph_targets_sampler.GetDimensions(tex_size, height, depth);

    for(int i = 0; i < WEIGHT_COUNT; ++i)
    {
        float4 displacment = get_displacment(anim_data, vertex_id, MORPH_TARGET_COLOR_OFFSET + i, tex_size);
        color += anim_data.morph_weights[i] * displacment;
    }
#endif

    return color;
}

#endif