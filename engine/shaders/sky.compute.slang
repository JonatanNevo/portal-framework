struct PushConstantsData
{
    float4 data1;
    float4 data2;
    float4 data3;
    float4 data4;
};

// Return random noise in the range [0.0, 1.0], as a function of x.
float Noise2d(float2 x)
{
    float xhash = cos( x.x * 37.0 );
    float yhash = cos( x.y * 57.0 );
    return frac( 415.92653 * ( xhash + yhash ) );
}

// Convert Noise2d() into a "star field" by stomping everthing below fThreshhold to zero.
float NoisyStarField(float2 vSamplePos, float fThreshhold )
{
    float StarVal = Noise2d( vSamplePos );
    if ( StarVal >= fThreshhold )
        StarVal = pow( (StarVal - fThreshhold)/(1.0 - fThreshhold), 6.0 );
    else
        StarVal = 0.0;
    return StarVal;
}

// Stabilize NoisyStarField() by only sampling at integer values.
float StableStarField( float2 vSamplePos, float fThreshhold )
{
    // Linear interpolation between four samples.
    // Note: This approach has some visual artifacts.
    // There must be a better way to "anti alias" the star field.
    float fractX = fract( vSamplePos.x );
    float fractY = fract( vSamplePos.y );
    float2 floorSample = floor( vSamplePos );    
    float v1 = NoisyStarField( floorSample, fThreshhold );
    float v2 = NoisyStarField( floorSample + float2( 0.0, 1.0 ), fThreshhold );
    float v3 = NoisyStarField( floorSample + float2( 1.0, 0.0 ), fThreshhold );
    float v4 = NoisyStarField( floorSample + float2( 1.0, 1.0 ), fThreshhold );

    float StarVal =   v1 * ( 1.0 - fractX ) * ( 1.0 - fractY )
        			+ v2 * ( 1.0 - fractX ) * fractY
        			+ v3 * fractX * ( 1.0 - fractY )
        			+ v4 * fractX * fractY;
	return StarVal;
}

void mainImage( out float4 fragColor, in float2 fragCoord, in RWTexture2D<float4> image, in PushConstantsData push_constants )
{
    float2 iResolution;
    image.GetDimensions(iResolution.x, iResolution.y);

	// Sky Background Color
	//vec3 vColor = vec3( 0.1, 0.2, 0.4 ) * fragCoord.y / iResolution.y;
    float3 vColor = push_constants.data1.xyz * fragCoord.y / iResolution.y;

    // Note: Choose fThreshhold in the range [0.99, 0.9999].
    // Higher values (i.e., closer to one) yield a sparser starfield.
    float StarFieldThreshhold = push_constants.data1.w;//0.97;

    // Stars with a slow crawl.
    float xRate = 0.2;
    float yRate = -0.06;
    float2 vSamplePos = fragCoord.xy + float2( xRate * float( 1 ), yRate * float( 1 ) );
	float StarVal = StableStarField( vSamplePos, StarFieldThreshhold );
    vColor += float3( StarVal );
	
	fragColor = float4(vColor, 1.0);
}


[shader("compute")]
[numthreads(16,16,1)]
void compute_main(uint3 dispatch_thread_id : SV_DispatchThreadID, uint3 group_thread_id : SV_GroupThreadID, RWTexture2D<float4> image, [[push_constant]] uniform PushConstantsData push_constants)
{
    uint2 texelCoord = dispatch_thread_id.xy;
    uint2 imageDims;
    image.GetDimensions(imageDims.x, imageDims.y);
    if (texelCoord.x < imageDims.x && texelCoord.y < imageDims.y)
    {
        float4 color;
        mainImage(color, float2(texelCoord), image, push_constants);
        
        image[texelCoord] = color;
    }
}