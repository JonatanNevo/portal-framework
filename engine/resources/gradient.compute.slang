struct PushConstants
{
    float4 data1;
    float4 data2;
    float4 data3;
    float4 data4;
};


[shader("compute")]
[numthreads(16,16,1)]
void compute_main(uint3 dispatch_thread_id : SV_DispatchThreadID, uint3 group_thread_id : SV_GroupThreadID, RWTexture2D<float4> image, [[push_constant]] uniform PushConstants push_constants)
{
    uint2 texel_coords = dispatch_thread_id.xy;

    uint2 size;
    image.GetDimensions(size.x, size.y);

    float4 top_color = push_constants.data1;
    float4 bottom_color = push_constants.data2;

    if(texel_coords.x < size.x && texel_coords.y < size.y)
    {
        float blend = float(texel_coords.y) / float(size.y);
        
        image[texel_coords] = lerp(top_color, bottom_color, blend);
    }
}