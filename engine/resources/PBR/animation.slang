module animation;

public extern static const int weight_count;
public extern static const int morph_target_position_offset;
public extern static const int morph_target_normal_offset;
public extern static const int morph_target_texcoord0_offset;
public extern static const int morph_target_texcoord1_offset;
public extern static const int morph_target_tangent_offset;
public extern static const int morph_target_color_offset;

public extern static const bool has_morph_targets;
public extern static const bool has_morph_target_position;
public extern static const bool has_morph_target_normal;
public extern static const bool has_morph_target_texcoord0;
public extern static const bool has_morph_target_texcoord1;
public extern static const bool has_morph_target_tangent;
public extern static const bool has_morph_target_color;
public extern static const bool use_morphing;
public extern static const bool has_joints_0_vec4;
public extern static const bool has_joints_1_vec4;
public extern static const bool has_weights_0_vec4;
public extern static const bool has_weights_1_vec4;
public extern static const bool use_skinning;

float4x4 get_matrix_from_texture(Sampler2D sampler, int index)
{
    float4x4 result = float4x4(1.0f);

    uint width, height;
    sampler.GetDimensions(width, height);
    int pixel_index = index * 4;

    for (int i = 0; i < 4; ++i)
    {
        int x = (pixel_index + i) % width;
        // Rounding mode of integers is undefined:
        // https://www.khronos.org/registry/OpenGL/specs/es/3.0/GLSL_ES_Specification_3.00.pdf (section 12.33)
        int y = (pixel_index + i) / width;
        result[i] = sampler.Load(int3(x, y, 0));
    }
    return result;
}

public struct AnimationData
{
    public Conditional<Sampler2DArray, has_morph_targets> morph_targets_sampler;

    public Conditional<Array<float, weight_count>, use_morphing> morph_weights;
    
    public Conditional<float4, has_joints_0_vec4> joints_0;
    public Conditional<float4, has_joints_1_vec4> joints_1;

    public Conditional<float4, has_weights_0_vec4> weights_0;
    public Conditional<float4, has_weights_1_vec4> weights_1;

    public Conditional<Sampler2D, use_skinning> joints_sampler;

    public float4x4 get_skinning_matrix()
    {
        float4x4 skin = float4x4(0.0f);

        if (has_weights_0_vec4 && has_joints_0_vec4)
        {
            skin +=
                weights_0.get().value.x * get_matrix_from_texture(joints_sampler.get().value, int(joints_0.get().value.x) * 2) +
                weights_0.get().value.y * get_matrix_from_texture(joints_sampler.get().value, int(joints_0.get().value.y) * 2) +
                weights_0.get().value.z * get_matrix_from_texture(joints_sampler.get().value, int(joints_0.get().value.z) * 2) +
                weights_0.get().value.w * get_matrix_from_texture(joints_sampler.get().value, int(joints_0.get().value.w) * 2);
        }

        if (has_weights_1_vec4 && has_joints_1_vec4)
        {
            skin +=
                weights_1.get().value.x * get_matrix_from_texture(joints_sampler.get().value, int(joints_1.get().value.x) * 2) +
                weights_1.get().value.y * get_matrix_from_texture(joints_sampler.get().value, int(joints_1.get().value.y) * 2) +
                weights_1.get().value.z * get_matrix_from_texture(joints_sampler.get().value, int(joints_1.get().value.z) * 2) +
                weights_1.get().value.w * get_matrix_from_texture(joints_sampler.get().value, int(joints_1.get().value.w) * 2);
        }

        if (all(skin == float4x4(0.0f))) {
            return float4x4(1.0f);
        }

        return skin;
    }

    public float4x4 get_skinning_normal_matrix()
    {
        float4x4 skin = float4x4(0.0f);

        if (has_weights_0_vec4 && has_joints_0_vec4)
        {
            skin +=
                weights_0.get().value.x * get_matrix_from_texture(joints_sampler.get().value, int(joints_0.get().value.x) * 2 + 1) +
                weights_0.get().value.y * get_matrix_from_texture(joints_sampler.get().value, int(joints_0.get().value.y) * 2 + 1) +
                weights_0.get().value.z * get_matrix_from_texture(joints_sampler.get().value, int(joints_0.get().value.z) * 2 + 1) +
                weights_0.get().value.w * get_matrix_from_texture(joints_sampler.get().value, int(joints_0.get().value.w) * 2 + 1);
        }

        if (has_weights_1_vec4 && has_joints_1_vec4)
        {
            skin +=
                weights_1.get().value.x * get_matrix_from_texture(joints_sampler.get().value, int(joints_1.get().value.x) * 2 + 1) +
                weights_1.get().value.y * get_matrix_from_texture(joints_sampler.get().value, int(joints_1.get().value.y) * 2 + 1) +
                weights_1.get().value.z * get_matrix_from_texture(joints_sampler.get().value, int(joints_1.get().value.z) * 2 + 1) +
                weights_1.get().value.w * get_matrix_from_texture(joints_sampler.get().value, int(joints_1.get().value.w) * 2 + 1);
        }

        if (all(skin == float4x4(0.0f))) {
            return float4x4(1.0f);
        }

        return skin;
    }

    public float4 get_displacment(int vertex_id, int target_index, int tex_size)
    {
        if (!has_morph_targets)
        {
            return float4(0.f);
        }

        int x = vertex_id % tex_size;
        // Rounding mode of integers is undefined:
        // https://www.khronos.org/registry/OpenGL/specs/es/3.0/GLSL_ES_Specification_3.00.pdf (section 12.33)
        int y = (vertex_id - x) / tex_size;
        return morph_targets_sampler.get().value.Load( { x, y, target_index });
    }

    public float4 get_target_position(int vertex_id)
    {
        float4 position = float4(0.f);

        if (has_morph_target_position)
        {
            uint tex_size, height, depth;
            morph_targets_sampler.get().value.GetDimensions(tex_size, height, depth);

            for (int i = 0; i < weight_count; ++i)
            {
                float4 displacment = get_displacment(vertex_id, morph_target_position_offset + i, tex_size);
                position += morph_weights.get().value[i] * displacment;
            }
        }

        return position;
    }

    public float3 get_target_normal(int vertex_id)
    {
        float3 normal = float3(0.f);

        if (has_morph_target_normal)
        {
            uint tex_size, height, depth;
            morph_targets_sampler.get().value.GetDimensions(tex_size, height, depth);

            for (int i = 0; i < weight_count; ++i)
            {
                float4 displacment = get_displacment(vertex_id, morph_target_normal_offset + i, tex_size);
                normal += (morph_weights.get().value[i] * displacment).xyz;
            }
        }

        return normal;
    }

    public float2 get_target_tex_coord0(int vertex_id)
    {
        float2 uv = float2(0.f);

        if (has_morph_target_texcoord0)
        {
            uint tex_size, height, depth;
            morph_targets_sampler.get().value.GetDimensions(tex_size, height, depth);

            for (int i = 0; i < weight_count; ++i)
            {
                float4 displacment = get_displacment(vertex_id, morph_target_texcoord0_offset + i, tex_size);
                uv += morph_weights.get().value[i] * displacment.xy;
            }
        }

        return uv;
    }

    public float2 get_target_tex_coord1(int vertex_id)
    {
        float2 uv = float2(0.f);

        if (has_morph_target_texcoord1)
        {
            uint tex_size, height, depth;
            morph_targets_sampler.get().value.GetDimensions(tex_size, height, depth);

            for (int i = 0; i < weight_count; ++i)
            {
                float4 displacment = get_displacment(vertex_id, morph_target_texcoord1_offset + i, tex_size);
                uv += morph_weights.get().value[i] * displacment.xy;
            }
        }

        return uv;
    }

    public float3 get_target_tangent(int vertex_id)
    {
        float3 tangent = float3(0.f);

        if (has_morph_target_tangent)
        {
            uint tex_size, height, depth;
            morph_targets_sampler.get().value.GetDimensions(tex_size, height, depth);

            for (int i = 0; i < weight_count; ++i)
            {
                float4 displacment = get_displacment(vertex_id, morph_target_tangent_offset + i, tex_size);
                tangent += morph_weights.get().value[i] * displacment.xyz;
            }
        }

        return tangent;
    }

    public float4 get_target_color(int vertex_id)
    {
        float4 color = float4(0.f);

        if (has_morph_target_color)
        {
            uint tex_size, height, depth;
            morph_targets_sampler.get().value.GetDimensions(tex_size, height, depth);

            for (int i = 0; i < weight_count; ++i)
            {
                float4 displacment = get_displacment(vertex_id, morph_target_color_offset + i, tex_size);
                color += morph_weights.get().value[i] * displacment;
            }
        }

        return color;
    }
}
