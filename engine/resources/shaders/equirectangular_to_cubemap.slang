import portal_common;

float3 GetCubeMapTexCoord(float2 imageSize, uint3 dispatchThreadID)
{
    float2 st = dispatchThreadID.xy / imageSize;
    float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - float2(1.0);

    float3 ret;
    if (dispatchThreadID.z == 0)      ret = float3(  1.0, uv.y, -uv.x);
    else if (dispatchThreadID.z == 1) ret = float3( -1.0, uv.y,  uv.x);
    else if (dispatchThreadID.z == 2) ret = float3( uv.x,  1.0, -uv.y);
    else if (dispatchThreadID.z == 3) ret = float3( uv.x, -1.0,  uv.y);
    else if (dispatchThreadID.z == 4) ret = float3( uv.x, uv.y,   1.0);
    else if (dispatchThreadID.z == 5) ret = float3(-uv.x, uv.y,  -1.0);
    return normalize(ret);
}

// ---------------------------------------------------------------------------------------------------
// Converts equirectangular (lat-long) projection texture into a cube-map

[vk::binding(0)]
[vk::image_format("rgba16f")]
RWTextureCube<float4> o_CubeMap;


[shader("compute")]
[numthreads(32, 32, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, Sampler2D EquirectangularTex)
{
    int width, height;
    o_CubeMap.GetDimensions(width, height);

    float3 cubeTC = GetCubeMapTexCoord(float2(width, height), dispatchThreadID);

    // Calculate sampling coords for equirectangular texture
    // https://en.wikipedia.org/wiki/Spherical_coordinate_system#Cartesian_coordinates
	float phi = atan2(cubeTC.z, cubeTC.x);
	float theta = acos(cubeTC.y);
	float2 uv = float2(phi / TWO_PI + 0.5, theta / PI);

    float4 color = EquirectangularTex.Sample(uv);
    color = min(color, float4(500.f));
    o_CubeMap[dispatchThreadID] = color;
}