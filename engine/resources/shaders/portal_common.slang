module portal_common;

//////////////////////////////////////////
// Consts
//////////////////////////////////////////

public const static float PI = 3.141592;
public const static float TWO_PI = PI * 2;
public const static float EPSILON = 0.00001f;


//////////////////////////////////////////
// Functions
//////////////////////////////////////////

public float sqr(float x) 
{
    return x * x;
}

public float3 rotate_vector_about_y(float angle, float3 vec)
{
    angle = radians(angle);
    float3x3 rotation_matrix = { float3(cos(angle),0.0,sin(angle)),
							    float3(0.0,1.0,0.0),
							    float3(-sin(angle),0.0,cos(angle)) };
    return mul(rotation_matrix, vec);
}

public float convert_sRGB_from_linear(float linear)
{
    	return linear <= 0.0031308f
		? linear * 12.92f
		: pow(linear, 1.0f / 2.4f) * 1.055f - 0.055f;
}

public float4 to_linear(float4 srgb)
{
    bool4 cutoff = srgb < 0.04045f;
    float4 higher = pow((srgb + 0.055f) / 1.055f, float4(2.4f));
    float4 lower = srgb / float4(12.92f);
    return lerp(higher, lower, cutoff);
}