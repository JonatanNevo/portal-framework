module portal_scene;

public struct SceneCamera
{
    public float4x4 view_proj;
    public float4x4 inverse_view_proj;
    public float4x4 proj;
    public float4x4 inverse_proj;
    public float4x4 view;
    public float4x4 inverse_view;
}

public struct ScreenData
{
    public float2 inv_full_resolution;
    public float2 full_resulution;
    public float2 inv_half_resolution;
    public float2 half_resolution;
}

public struct RendererData
{
    public int tiles_count_x;
}

public struct SceneData
{
    public SceneCamera camera;
    public ScreenData screen_data;
    public RendererData renderer_data;

    public SamplerCube environment_radiance_texture;
    public SamplerCube environment_irradiance_texture;
    public Sampler2D brdf_lut_texture;
}