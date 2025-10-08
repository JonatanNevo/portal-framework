module iridescence;

import functions;
import brdf;

// XYZ to sRGB color space (transposed for column-major)
static const float3x3 XYZ_TO_REC709 = {
     3.2404542, -1.5371385, -0.4985314,
    -0.9692660,  1.8760108,  0.0415560,
     0.0556434, -0.2040259,  1.0572252
};

// Assume air interface for top
// Note: We don't handle the case fresnel0 == 1
public float3 fresnel0_to_ior(float3 fresnel0)
{
    float3 sqrtF0 = sqrt(fresnel0);
    return (float3(1.0) + sqrtF0) / (float3(1.0) - sqrtF0);
}

// Conversion FO/IOR
public float3 ior_to_fresnel0(float3 transmitted_ior, float incident_ior)
{
    return sq((transmitted_ior - float3(incident_ior)) / (transmitted_ior + float3(incident_ior)));
}

// ior is a value between 1.0 and 3.0. 1.0 is air interface
public float ior_to_fresnel0(float transmitted_ior, float incident_ior)
{
    return sq((transmitted_ior - incident_ior) / (transmitted_ior + incident_ior));
}

// Fresnel equations for dielectric/dielectric interfaces.
// Ref: https://belcour.github.io/blog/research/2017/05/01/brdf-thin-film.html
// Evaluation XYZ sensitivity curves in Fourier space
public float3 eval_sensitivity(float opd, float3 shift)
{
    float phase = 2.0 * M_PI * opd * 1.0e-9;
    float3 val = float3(5.4856e-13, 4.4201e-13, 5.2481e-13);
    float3 pos = float3(1.6810e+06, 1.7953e+06, 2.2084e+06);
    float3 var = float3(4.3278e+09, 9.3046e+09, 6.6121e+09);

    float3 xyz = val * sqrt(2.0 * M_PI * var) * cos(pos * phase + shift) * exp(-sq(phase) * var);
    xyz.x += 9.7470e-14 * sqrt(2.0 * M_PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift[0]) * exp(-4.5282e+09 * sq(phase));
    xyz /= 1.0685e-7;

    float3 srgb = mul(XYZ_TO_REC709, xyz);
    return srgb;
}

public float3 eval_iridescence(float outside_ior, float eta2, float cos_theta1, float thin_film_thickness, float3 base_f0)
{
    float3 I;

    // Force iridescenceIor -> outsideIOR when thinFilmThickness -> 0.0
    float iridescenceIor = lerp(outside_ior, eta2, smoothstep(0.0, 0.03, thin_film_thickness));
    // Evaluate the cosTheta on the base layer (Snell law)
    float sinTheta2Sq = sq(outside_ior / iridescenceIor) * (1.0 - sq(cos_theta1));

    // Handle TIR:
    float cosTheta2Sq = 1.0 - sinTheta2Sq;
    if (cosTheta2Sq < 0.0) {
        return float3(1.0);
    }

    float cosTheta2 = sqrt(cosTheta2Sq);

    // First interface
    float R0 = ior_to_fresnel0(iridescenceIor, outside_ior);
    float R12 = f_schlick(R0, cos_theta1);
    float R21 = R12;
    float T121 = 1.0 - R12;
    float phi12 = 0.0;
    if (iridescenceIor < outside_ior) phi12 = M_PI;
    float phi21 = M_PI - phi12;

    // Second interface
    float3 baseIOR = fresnel0_to_ior(clamp(base_f0, 0.0, 0.9999)); // guard against 1.0
    float3 R1 = ior_to_fresnel0(baseIOR, iridescenceIor);
    float3 R23 = f_schlick(R1, cosTheta2);
    float3 phi23 = float3(0.0);
    if (baseIOR[0] < iridescenceIor) phi23[0] = M_PI;
    if (baseIOR[1] < iridescenceIor) phi23[1] = M_PI;
    if (baseIOR[2] < iridescenceIor) phi23[2] = M_PI;

    // Phase shift
    float OPD = 2.0 * iridescenceIor * thin_film_thickness * cosTheta2;
    float3 phi = float3(phi21) + phi23;

    // Compound terms
    float3 R123 = clamp(R12 * R23, 1e-5, 0.9999);
    float3 r123 = sqrt(R123);
    float3 Rs = sq(T121) * R23 / (float3(1.0) - R123);

    // Reflectance term for m = 0 (DC term amplitude)
    float3 C0 = R12 + Rs;
    I = C0;

    // Reflectance term for m > 0 (pairs of diracs)
    float3 Cm = Rs - T121;
    for (int m = 1; m <= 2; ++m)
    {
        Cm *= r123;
        float3 Sm = 2.0 * eval_sensitivity(float(m) * OPD, float(m) * phi);
        I += Cm * Sm;
    }

    // Since out of gamut colors might be produced, negative color values are clamped to 0.
    return max(I, float3(0.0));
}