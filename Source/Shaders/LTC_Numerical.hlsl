//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com



// LINEARLY TRANSFORMED COSINES (LTC)
//
// "Real-Time Polygonal-Light Shading with Linearly Transformed Cosines", by Eric Heitz et al,
// https://eheitzresearch.wordpress.com/415-2/
//
// sources: 
//
// - GPU Zen
// - https://blog.selfshadow.com/publications/s2017-shading-course/#course_content
// - Paper: https://drive.google.com/file/d/0BzvWIdpUpRx_d09ndGVjNVJzZjA/view
// - Slides: https://drive.google.com/file/d/0BzvWIdpUpRx_Z2pZWWFtam5xTFE/view
//

#include "LTC_Utils.hlsl"
#include "LTC_Numerical.hlsl"
#include "ShadingMath.hlsl"

#define SAFE_DIV_EPSILON 0.00001f
#define SAFE_DIV(expr) max(expr, SAFE_DIV_EPSILON)


// Illumination = Integral( BRDF * cos(theta) * dw );
//
// D = BRDF * cos
//
float D(float3 w, in float3x3 Minv)
{
	// ~ brdf * cos
    float3 wo = mul(Minv, w);
    //float3 wo = w;
    float lo = length(wo);

	// reference implementation (https://blog.selfshadow.com/ltc/webgl/ltc_line.html) uses wo.z
	// but we have to use wo.y on wo right below. not sure why :(
    return 1.0f / PI * (saturate(wo.y / lo) * abs(determinant(Minv))) / (lo * lo * lo);
}

float D(float3 w)
{
    float3x3 Minv = float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);
    return D(w, Minv);
}

//--------------------------------------------------------------------------------
// UTILITIES
//--------------------------------------------------------------------------------
float SamplePhi(int i, int iMax) { return TWO_PI * float(i) / float(iMax); }

// code from [Frisvad2012]
void BuildOrthonormalBasis(float3 N, out float3 T, out float3 B)
{
	if(N.z < -0.999999)
    {
        T = float3(0, -1, 0);
        B = float3(-1, 0, 0);
        return;
    }
    float a = 1.0f / (1.0f + N.z);
    float b = -N.x * N.y * a;
    T = float3(1.0f - N.x * N.x * a, b, -N.x);
    B = float3(b, 1.0f - N.y * N.y * a, -N.y);
}

// LUT is 32x32
#define LTC_LUT_SIZE 32.0f
float2 GetLTC_UVs(float cosTheta, float roughness)
{
    const float theta = acos(cosTheta);
    float2 uv = float2(roughness, theta / (0.5f - PI));

	// scale + bias for proper filtering
    uv = uv * (LTC_LUT_SIZE - 1.0f) / LTC_LUT_SIZE + (0.5 / LTC_LUT_SIZE);
    return uv;
}

float3x3 GetLTC_Matrix(Texture2D texLUT, SamplerState linearSampler, float2 uv)
{
    const float4 t = texLUT.Sample(linearSampler, uv);
#if 1
    return float3x3(
		  1.0f,  0.0f,  t.y
		, 0.0f,  t.z ,  0.0f
	    , t.w ,  0.0f,  t.x
	);
#else
    return float3x3(
                float3(t.x, 0, t.y),
                float3(0, 1, 0),
                float3(t.z, 0, t.w)
            );
#endif
}

//--------------------------------------------------------------------------------
// NUMERICAL INTEGRATIONS
//--------------------------------------------------------------------------------
float I_cylinder_numerical(float3 p1, float3 p2, float R, in float3x3 matTBN, in float3x3 Minv)
{
	// Initialize the orthonormal basis vectors
    float L = length(p2 - p1);
    float3 wt = normalize(p2 - p1);
    float3 wt1, wt2;
    BuildOrthonormalBasis(wt, wt1, wt2);
	
	// illumination
    float I = 0.0f; 

	// integral discretization
    const int nSamplesPhi = 20;
    const int nSamplesL   = 100;
    for (int i = 0; i < nSamplesPhi; ++i)
    {
        for (int j = 0; j < nSamplesL; ++j)
        {
			// normal vector
            float phi = SamplePhi(i, nSamplesPhi);
            float3 wn = cos(phi) * wt1 + sin(phi) * wt2;

			// position
            float l = L * float(j) / float(nSamplesL);
            float3 p = p1 + l * wt + R * wn;

			// normalize the direction
            float3 wp = normalize(p);

			// integrate
            //I += D(wp) * saturate(dot(-wp, wn)) / dot(p, p);
            I += D(wp, transpose(matTBN)) * saturate(dot(-wp, wn)) / dot(p, p);
        }
    }

    I *= TWO_PI * R * L / float(nSamplesPhi * nSamplesL);
	return I;
}

// NOT WORKING
float I_line_numerical(float3 p1, float3 p2, in float3x3 matTBN, in float3x3 Minv)
{
    float L = length(p2 - p1);
    float3 wt = normalize(p2 - p1);

	// illumination
    float I = 0.0f;

	// integral discretization
    const int nSamples = 500;
    for (int i = 0; i < nSamples; ++i)
    {
		// position
        float3 p = p1 + L * float(i) / float(nSamples - 1) * wt;

		// normalize the direction
        float wp = normalize(p);

		// integrate
        //I += 2.0f * D(wp) * length(cross(wp, wt)) / dot(p, p);
        I += 2.0f * D(wp, transpose(matTBN)) * length(cross(wp, wt)) / dot(p, p);
    }

    I *= L / float(nSamples);
    return I;
}


float I_disks_numerical(float3 p1, float3 p2, float R)
{
	// orthonormal base
    float L = length(p2 - p1);
    float3 wt = normalize(p2 - p1);
    float3 wt1, wt2;
    BuildOrthonormalBasis(wt, wt1, wt2);

	// illumination
    float Idisks = 0.0f;

	// integration
    const int nSamplesPhi = 20;
    const int nSamplesR = 200;
    for (int i = 0; i < nSamplesPhi; ++i)
    {
        for (int j = 0; j < nSamplesR; ++j)
        {
            float phi = SamplePhi(i, nSamplesPhi);
            float r = R * float(j) / float(nSamplesR - 1);
            float3 p, wp;

            p = p1 + r * (cos(phi) * wt1 + sin(phi) * wt2);
            wp = normalize(p);
            Idisks += r * D(wp) * saturate(dot(wp, +wt)) * dot(p, p);
			
            p = p2 + r * (cos(phi) * wt1 + sin(phi) * wt2);
            wp = normalize(p);
            Idisks += r * D(wp) * saturate(dot(wp, -wt)) * dot(p, p);
        }
    }

    Idisks *= TWO_PI * R / float(nSamplesPhi * nSamplesR);
	return Idisks;
}

// Most accurate with:
//
// - cylinders of small radius
// - cylinders far from the shading point
// - low frequency sampling (high roughness materials)
//
float I_cylinder_approx(float3 p1, float3 p2, float R, in float3x3 matTBN, in float3x3 Minv)
{
	// if the radius @R is small enough, we can use the line integral as an approximation.
	// 
	//		I_cyl(R) ~ R * I_line
	//
	// In cases where @R is too large, the approximation could be inaccurate and overshoot.
	// As we use normalized distributions, we can clamp it to 1.0f to prevent overly high values.
	//
    return min(1.0f, R * I_line_numerical(p1, p2, matTBN, Minv));
}



//--------------------------------------------------------------------------------
// LTC INTEGRATIONS
//--------------------------------------------------------------------------------
float Fpo(float d, float l)
{
    const float d2 = d * d; 
	const float l2 = l * l;
    return l / (d * (d2 + l2)) + atan(l / d) / d2;
}
float Fwt(float d, float l)
{
    const float d2 = d * d;
    const float l2 = l * l;
    return l2 / (d * (d2 + l2));
}
float I_diffuse_line(float3 p1, float3 p2)
{
	// tangent
    float3 wt = normalize(p2 - p1);

	// clamping
    if (p1.z <= 0.0f && p2.z < 0.0f)	return 0.0f;	// clamp below horizon
	if (p1.z <= 0.0f)	p1 = (+p1 * p2.z - p2 * p1.z) / (+p2.z - p1.z);
    if (p1.z <= 0.0f)	p2 = (-p1 * p2.z + p2 * p1.z) / (-p2.z + p1.z);

	// shading point orthonormal projection on line
    float3 po = p1 - wt * dot(p1, wt);

	// distance to line
    float d = length(po);

	// parameterization
    float l1 = dot(p1 - po, wt);
    float l2 = dot(p2 - po, wt);

	// integral
    float I =	(Fpo(d, l2) - Fpo(d, l1)) * po.z +
				(Fwt(d, l2) - Fwt(d, l1)) * wt.z;

    return I / PI;
}


float I_ltc_line(float3 p1, float3 p2, in float3x3 Minv, in float3x3 matTBN)
{
    float3 p1o = mul(Minv, p1);
    float3 p2o = mul(Minv, p2);

    float I_diffuse = I_diffuse_line(p1o, p2o);

    float3 ortho = normalize(cross(p1, p2));
    float w = 1.0f / length(mul(inverse(transpose(Minv)), ortho));

	return w * I_diffuse;
}

float I_ltc_line_rectangle(float3 p1, float3 p2, float3 wn, float3x3 Minv)
{
    float3 wortho = normalize(cross(p1, p2));
    float I = abs(dot(wortho, wn)) * I_ltc_line(p1, p2, Minv, Minv /* TODO */);
    return I;
}

float I_ltc_disks(float3 p1, float3 p2, float R, float3x3 Minv)
{
    float A = PI * R * R;
	
    float3 wt = normalize(p2 - p1);
    float3 wp1 = normalize(p1);
    float3 wp2 = normalize(p2);
	
    float Idisks = A * (
			D(wp1, Minv) * saturate(dot(+wt, wp1)) / dot(p1, p2) +
			D(wp2, Minv) * saturate(dot(-wt, wp2)) / dot(p2, p2)
		);
	return Idisks;
}




//--------------------------------------------------------------------------------
// AREA LIGHT ILLUMINATION
//--------------------------------------------------------------------------------
float f(float NdotV, float roughness)
{
    const float2 ltc_uvs = GetLTC_UVs(NdotV, roughness);
    //const float3x3 Minv = GetLTC_Matrix(texLUT, smpLinear, ltc_uvs);

}

// determines if the space of @s.N and @V vectors 
// will be in view space or world space.
#define LTC_USE_VIEW_SPACE 0
float3 EvalCylinder(BRDF_Surface s, float3 V, CylinderLight l, Texture2D texLUT, SamplerState smpLinear, in matrix matView)
{
	// get inverse transform matrix
    const float NdotV = saturate(dot(s.N, V));
    const float2 ltc_uvs = GetLTC_UVs(NdotV, s.roughness);
    const float3x3 Minv = GetLTC_Matrix(texLUT, smpLinear, ltc_uvs);

	// generate cylidner points
    float3 halfHeightVec = float3(l.height * 0.5f, 0, 0);
    //float3 halfHeightVec = float3(0, l.height * 0.5f, 0);
    //float3 halfHeightVec = float3(0, 0, l.height * 0.5f);

    float4 p1W = float4(l.position + halfHeightVec, 1.0f);
    float4 p2W = float4(l.position - halfHeightVec, 1.0f);
    
    float3 T1 = normalize(V - dot(s.N, V) * s.N);
    float3 T2 = cross(s.N, T1);
    float3x3 matTBN = transpose(float3x3
		(T1, T2, s.N)
		//(T2, T1, s.N)
	);

	
#if LTC_USE_VIEW_SPACE
    float3 p1V = mul(matView, p1W);
    float3 p2V = mul(matView, p2W);
    float3 p1 = mul(matTBN, float3(p1V.xyz - s.P.xyz));
    float3 p2 = mul(matTBN, float3(p2V.xyz - s.P.xyz));
#else
    float3 p1 = mul(matTBN, float3(p1W.xyz - s.P.xyz));
    float3 p2 = mul(matTBN, float3(p2W.xyz - s.P.xyz));
#endif
	
    float3 wp1 = normalize(p1);
    float3 wp2 = normalize(p2);
    
    //return -normalize(p1W - s.P);
    //return NdotV;
    //return V / 10;
	//return s.P/ 1000;
	
    //return NdotV.xxx;

#if 0
    float3 I = l.brightness * l.color * I_ltc_line(p1, p2, Minv);
#else
    float3 I = l.brightness * l.color 
	//* I_cylinder_numerical(p1, p2, l.radius, matTBN, Minv);
	//* I_line_numerical(p1, p2, matTBN, Minv);
	//* I_ltc_line(p1, p2, matTBN);
	* I_ltc_line(p1, p2, Minv, transpose(matTBN));
	//* I_cylinder_approx(p1, p2, l.radius, matTBN, Minv);
#endif

    //return 0.0f; // temp
    //return max(0.0f, min(1.0f, I));
    return max(0.0f, I / TWO_PI);
    //return I;
    //return abs(I);
}



/////////////////
// ORIGINAL SOURCE: 
/////////////////
#if 0
// bind roughness   {label:"Roughness", default:0.25, min:0.01, max:1, step:0.001}
// bind dcolor      {label:"Diffuse Color",  r:1.0, g:1.0, b:1.0}
// bind scolor      {label:"Specular Color", r:1.0, g:1.0, b:1.0}
// bind intensity   {label:"Light Intensity", default:4, min:0, max:10}
// bind width       {label:"Width",  default: 8, min:0.1, max:15, step:0.1}
// bind height      {label:"Height", default: 8, min:0.1, max:15, step:0.1}
// bind roty        {label:"Rotation Y", default: 0, min:0, max:1, step:0.001}
// bind rotz        {label:"Rotation Z", default: 0, min:0, max:1, step:0.001}
// bind twoSided    {label:"Two-sided", default:false}

uniform float roughness;
uniform vec3 dcolor;
uniform vec3 scolor;

uniform float intensity;
uniform float width;
uniform float height;
uniform float roty;
uniform float rotz;

uniform bool twoSided;

uniform sampler2D ltc_mat;
uniform sampler2D ltc_mag;

uniform mat4 view;
uniform vec2 resolution;
uniform int sampleCount;

const float LUT_SIZE = 64.0;
const float LUT_SCALE = (LUT_SIZE - 1.0) / LUT_SIZE;
const float LUT_BIAS = 0.5 / LUT_SIZE;

const float pi = 3.14159265;

// Tracing and intersection
///////////////////////////

struct Ray
{
    vec3 origin;
    vec3 dir;
};

struct Rect
{
    vec3 center;
    vec3 dirx;
    vec3 diry;
    float halfx;
    float halfy;

    vec4 plane;
};

bool RayPlaneIntersect(Ray ray, vec4 plane, out float t)
{
    t = -dot(plane, vec4(ray.origin, 1.0)) / dot(plane.xyz, ray.dir);
    return t > 0.0;
}

bool RayRectIntersect(Ray ray, Rect rect, out float t)
{
    bool intersect = RayPlaneIntersect(ray, rect.plane, t);
    if (intersect)
    {
        vec3 pos = ray.origin + ray.dir * t;
        vec3 lpos = pos - rect.center;
        
        float x = dot(lpos, rect.dirx);
        float y = dot(lpos, rect.diry);

        if (abs(x) > rect.halfx || abs(y) > rect.halfy)
            intersect = false;
    }

    return intersect;
}

// Camera functions
///////////////////

Ray GenerateCameraRay(float u1, float u2)
{
    Ray ray;

    // Random jitter within pixel for AA
    vec2 xy = 2.0 * (gl_FragCoord.xy) / resolution - vec2(1.0);

    ray.dir = normalize(vec3(xy, 2.0));

    float focalDistance = 2.0;
    float ft = focalDistance / ray.dir.z;
    vec3 pFocus = ray.dir * ft;

    ray.origin = vec3(0);
    ray.dir = normalize(pFocus - ray.origin);

    // Apply camera transform
    ray.origin = (view * vec4(ray.origin, 1)).xyz;
    ray.dir = (view * vec4(ray.dir, 0)).xyz;

    return ray;
}

vec3 mul(mat3 m, vec3 v)
{
    return m * v;
}

mat3 mul(mat3 m1, mat3 m2)
{
    return m1 * m2;
}

vec3 rotation_y(vec3 v, float a)
{
    vec3 r;
    r.x = v.x * cos(a) + v.z * sin(a);
    r.y = v.y;
    r.z = -v.x * sin(a) + v.z * cos(a);
    return r;
}

vec3 rotation_z(vec3 v, float a)
{
    vec3 r;
    r.x = v.x * cos(a) - v.y * sin(a);
    r.y = v.x * sin(a) + v.y * cos(a);
    r.z = v.z;
    return r;
}

vec3 rotation_yz(vec3 v, float ay, float az)
{
    return rotation_z(rotation_y(v, ay), az);
}

mat3 transpose(mat3 v)
{
    mat3 tmp;
    tmp[0] = vec3(v[0].x, v[1].x, v[2].x);
    tmp[1] = vec3(v[0].y, v[1].y, v[2].y);
    tmp[2] = vec3(v[0].z, v[1].z, v[2].z);

    return tmp;
}

// Linearly Transformed Cosines
///////////////////////////////

float IntegrateEdge(vec3 v1, vec3 v2)
{
    float cosTheta = dot(v1, v2);
    float theta = acos(cosTheta);
    float res = cross(v1, v2).z * ((theta > 0.001) ? theta / sin(theta) : 1.0);

    return res;
}

void ClipQuadToHorizon(inout vec3 L[5], out int n)
{
    // detect clipping config
    int config = 0;
    if (L[0].z > 0.0)
        config += 1;
    if (L[1].z > 0.0)
        config += 2;
    if (L[2].z > 0.0)
        config += 4;
    if (L[3].z > 0.0)
        config += 8;

    // clip
    n = 0;

    if (config == 0)
    {
        // clip all
    }
    else if (config == 1) // V1 clip V2 V3 V4
    {
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 2) // V2 clip V1 V3 V4
    {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 3) // V1 V2 clip V3 V4
    {
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 4) // V3 clip V1 V2 V4
    {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    }
    else if (config == 5) // V1 V3 clip V2 V4) impossible
    {
        n = 0;
    }
    else if (config == 6) // V2 V3 clip V1 V4
    {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 7) // V1 V2 V3 clip V4
    {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 8) // V4 clip V1 V2 V3
    {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = L[3];
    }
    else if (config == 9) // V1 V4 clip V2 V3
    {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    }
    else if (config == 10) // V2 V4 clip V1 V3) impossible
    {
        n = 0;
    }
    else if (config == 11) // V1 V2 V4 clip V3
    {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 12) // V3 V4 clip V1 V2
    {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    }
    else if (config == 13) // V1 V3 V4 clip V2
    {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    }
    else if (config == 14) // V2 V3 V4 clip V1
    {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    }
    else if (config == 15) // V1 V2 V3 V4
    {
        n = 4;
    }
    
    if (n == 3)
        L[3] = L[0];
    if (n == 4)
        L[4] = L[0];
}


vec3 LTC_Evaluate(
    vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSided)
{
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = mul(Minv, transpose(mat3(T1, T2, N)));

    // polygon (allocate 5 vertices for clipping)
    vec3 L[5];
    L[0] = mul(Minv, points[0] - P);
    L[1] = mul(Minv, points[1] - P);
    L[2] = mul(Minv, points[2] - P);
    L[3] = mul(Minv, points[3] - P);

    int n;
    ClipQuadToHorizon(L, n);
    
    if (n == 0)
        return vec3(0, 0, 0);

    // project onto sphere
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);
    L[4] = normalize(L[4]);

    // integrate
    float sum = 0.0;

    sum += IntegrateEdge(L[0], L[1]);
    sum += IntegrateEdge(L[1], L[2]);
    sum += IntegrateEdge(L[2], L[3]);
    if (n >= 4)
        sum += IntegrateEdge(L[3], L[4]);
    if (n == 5)
        sum += IntegrateEdge(L[4], L[0]);

    sum = twoSided ? abs(sum) : max(0.0, sum);

    vec3 Lo_i = vec3(sum, sum, sum);

    return Lo_i;
}

// Scene helpers
////////////////

void InitRect(out Rect rect)
{
    rect.dirx = rotation_yz(vec3(1, 0, 0), roty * 2.0 * pi, rotz * 2.0 * pi);
    rect.diry = rotation_yz(vec3(0, 1, 0), roty * 2.0 * pi, rotz * 2.0 * pi);

    rect.center = vec3(0, 6, 32);
    rect.halfx = 0.5 * width;
    rect.halfy = 0.5 * height;

    vec3 rectNormal = cross(rect.dirx, rect.diry);
    rect.plane = vec4(rectNormal, -dot(rectNormal, rect.center));
}

void InitRectPoints(Rect rect, out vec3 points[4])
{
    vec3 ex = rect.halfx * rect.dirx;
    vec3 ey = rect.halfy * rect.diry;

    points[0] = rect.center - ex - ey;
    points[1] = rect.center + ex - ey;
    points[2] = rect.center + ex + ey;
    points[3] = rect.center - ex + ey;
}

// Misc. helpers
////////////////

float saturate(float v)
{
    return clamp(v, 0.0, 1.0);
}

vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float gamma = 2.2;

vec3 ToLinear(vec3 v)
{
    return PowVec3(v, gamma);
}
vec3 ToSRGB(vec3 v)
{
    return PowVec3(v, 1.0 / gamma);
}

void main()
{
    Rect rect;
    InitRect(rect);

    vec3 points[4];
    InitRectPoints(rect, points);

    vec4 floorPlane = vec4(0, 1, 0, 0);

    vec3 lcol = vec3(intensity);
    vec3 dcol = ToLinear(dcolor);
    vec3 scol = ToLinear(scolor);
    
    vec3 col = vec3(0);

    Ray ray = GenerateCameraRay(0.0, 0.0);

    float distToFloor;
    bool hitFloor = RayPlaneIntersect(ray, floorPlane, distToFloor);
    if (hitFloor)
    {
        vec3 pos = ray.origin + ray.dir * distToFloor;

        vec3 N = floorPlane.xyz;
        vec3 V = -ray.dir;
        
        float theta = acos(dot(N, V));
        vec2 uv = vec2(roughness, theta / (0.5 * pi));
        uv = uv * LUT_SCALE + LUT_BIAS;
        
        vec4 t = 
        texture2D( ltc_mat, uv);
        mat3 Minv = mat3(
            vec3(1, 0, t.y),
            vec3(0, t.z, 0),
            vec3(t.w, 0, t.x)
        );
        
        vec3 spec = LTC_Evaluate(N, V, pos, Minv, points, twoSided);
        spec *= 
        texture2D( ltc_mag, uv).
        w;
        
        vec3
        diff = LTC_Evaluate(N, V, pos, mat3(1), points, twoSided);
        
        col = lcol * (scol * spec + dcol * diff);
        col /= 2.0 * pi;
    }

    float distToRect;
    if (RayRectIntersect(ray, rect, distToRect))
        if ((distToRect < distToFloor) || !hitFloor)
            col = lcol;

    gl_FragColor = vec4(col, 1.0);
}
#endif