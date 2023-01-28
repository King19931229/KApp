#ifndef _SHADOW_SAMPLE_H_
#define _SHADOW_SAMPLE_H_

const vec2 poisson16[] = vec2[](
	vec2( -0.94201624,  -0.39906216 ),
	vec2(  0.94558609,  -0.76890725 ),
	vec2( -0.094184101, -0.92938870 ),
	vec2(  0.34495938,   0.29387760 ),
	vec2( -0.91588581,   0.45771432 ),
	vec2( -0.81544232,  -0.87912464 ),
	vec2( -0.38277543,   0.27676845 ),
	vec2(  0.97484398,   0.75648379 ),
	vec2(  0.44323325,  -0.97511554 ),
	vec2(  0.53742981,  -0.47373420 ),
	vec2( -0.26496911,  -0.41893023 ),
	vec2(  0.79197514,   0.19090188 ),
	vec2( -0.24188840,   0.99706507 ),
	vec2( -0.81409955,   0.91437590 ),
	vec2(  0.19984126,   0.78641367 ),
	vec2(  0.14383161,  -0.14100790 )
);

float ZClipToEye(float lightZNear, float lightZFar, float z)
{
	return lightZFar * lightZNear / (lightZFar - z * (lightZFar - lightZNear));
}

// Using similar triangles from the surface point to the area light
vec2 SearchRegionRadiusUV(vec2 lightRadiusUV, float lightZNear, float zWorld)
{
	return lightRadiusUV * (zWorld - lightZNear) / zWorld;
}

// Using similar triangles between the area light, the blocking plane and the surface point
vec2 PenumbraRadiusUV(vec2 lightRadiusUV, float zReceiver, float zBlocker)
{
	return lightRadiusUV * (zReceiver - zBlocker) / zBlocker;
}

// Project UV size to the near plane of the light
vec2 ProjectToLightUV(float lightZNear, vec2 sizeUV, float zWorld)
{
	return sizeUV * lightZNear / zWorld;
}

// Derivatives of light-space depth with respect to texture2D coordinates
vec2 DepthGradient(vec2 uv, float z)
{
#ifdef COMPUTE_SHADER
	return vec2(0.0, 0.0);
#else
	vec2 dz_duv = vec2(0.0, 0.0);

	vec3 duvdist_dx = dFdx(vec3(uv,z));
	vec3 duvdist_dy = dFdy(vec3(uv,z));

	dz_duv.x = duvdist_dy.y * duvdist_dx.z;
	dz_duv.x -= duvdist_dx.y * duvdist_dy.z;

	dz_duv.y = duvdist_dx.x * duvdist_dy.z;
	dz_duv.y -= duvdist_dy.x * duvdist_dx.z;

	float det = (duvdist_dx.x * duvdist_dy.y) - (duvdist_dx.y * duvdist_dy.x);
	dz_duv /= det;

	return dz_duv;
#endif
}

float BiasedZ(float z0, vec2 dz_duv, vec2 offset)
{
	return z0 + dot(dz_duv, offset);
}

float BorderDepthTexture(sampler2D tex, vec2 uv)
{
	return ((uv.x <= 1.0) && (uv.y <= 1.0) &&
	 (uv.x >= 0.0) && (uv.y >= 0.0)) ? textureLod(tex, uv, 0.0).x : 0.0;
}

float BorderPCFTexture(sampler2D tex, vec3 uvz)
{
	if((uvz.x <= 1.0) && (uvz.y <= 1.0) && (uvz.x >= 0.0) && (uvz.y >= 0.0))
	{
		return textureLod(tex, uvz.xy, 0).x <= uvz.z ? 1.0 : 0.0; 
	}
	return ((uvz.z <= 1.0) ? 1.0 : 0.0);
}

// Returns average blocker depth in the search region, as well as the number of found blockers.
// Blockers are defined as shadow-map samples between the surface point and the light.
void FindBlocker(
	out float accumBlockerDepth, 
	out float numBlockers,
	out float maxBlockers,
	sampler2D shadowMap,
	vec2 uv,
	float z0,
	vec2 dz_duv,
	vec2 searchRegionRadiusUV)
{
	accumBlockerDepth = 0.0;
	numBlockers = 0.0;
	maxBlockers = 16.0;

	for (int i = 0; i < 16; ++i)
	{
		vec2 offset = poisson16[i] * searchRegionRadiusUV;
		float shadowMapDepth = BorderDepthTexture(shadowMap, uv + offset);
		float z = BiasedZ(z0, dz_duv, offset);
		if (shadowMapDepth < z)
		{
			accumBlockerDepth += shadowMapDepth;
			numBlockers++;
		}
	}
}

float Pcf_4x4(sampler2D shadowMap, vec2 uv, float z0)
{
	vec2 vInvShadowMapWH = vec2(1.0) / vec2(textureSize(shadowMap, 0));
	vec2 texelPos = uv / vInvShadowMapWH - vec2(0.5, 0.5);	// bias to be consistent with texture filtering hardware
	vec2 fraction = fract(texelPos);
	vec2 texelCenter = floor(texelPos) + vec2(0.5, 0.5);	// bias to get reliable texel center content
	vec2 sample00TexelCenter = (texelCenter - vec2(1, 1)) * vInvShadowMapWH;
	vec4 litZ = vec4(z0);

	vec4 values0, values1, values2, values3; // valuesi in row i from left to right: x,y,z,w
	values0.x  = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(0,0) * vInvShadowMapWH));
	values0.y  = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(1,0) * vInvShadowMapWH));
	values0.z  = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(2,0) * vInvShadowMapWH));
	values0.w  = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(3,0) * vInvShadowMapWH));
	values0 = step(values0, litZ.xxxx);
	values1.x = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(0,1) * vInvShadowMapWH));
	values1.y = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(1,1) * vInvShadowMapWH));
	values1.z = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(2,1) * vInvShadowMapWH));
	values1.w = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(3,1) * vInvShadowMapWH));
	values1 = step(values1, litZ.xxxx);
	values2.x = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(0,2) * vInvShadowMapWH));
	values2.y = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(1,2) * vInvShadowMapWH));
	values2.z = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(2,2) * vInvShadowMapWH));
	values2.w = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(3,2) * vInvShadowMapWH));
	values2 = step(values2, litZ.xxxx);
	values3.x = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(0,3) * vInvShadowMapWH));
	values3.y = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(1,3) * vInvShadowMapWH));
	values3.z = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(2,3) * vInvShadowMapWH));
	values3.w = BorderDepthTexture(shadowMap, (sample00TexelCenter + vec2(3,3) * vInvShadowMapWH));
	values3 = step(values3, litZ.xxxx);

	vec2 verticalLerp00 = mix(vec2(values0.x, values1.x), vec2(values0.y, values1.y), fraction.xx);
	float pcfResult00 = mix(verticalLerp00.x, verticalLerp00.y, fraction.y);
	vec2 verticalLerp10 = mix(vec2(values0.y, values1.y), vec2(values0.z, values1.z), fraction.xx);
	float pcfResult10 = mix(verticalLerp10.x, verticalLerp10.y, fraction.y);
	vec2 verticalLerp20 = mix(vec2(values0.z, values1.z), vec2(values0.w, values1.w), fraction.xx);
	float pcfResult20 = mix(verticalLerp20.x, verticalLerp20.y, fraction.y);

	vec2 verticalLerp01 = mix(vec2(values1.x, values2.x), vec2(values1.y, values2.y), fraction.xx);
	float pcfResult01 = mix(verticalLerp01.x, verticalLerp01.y, fraction.y);
	vec2 verticalLerp11 = mix(vec2(values1.y, values2.y), vec2(values1.z, values2.z), fraction.xx);
	float pcfResult11 = mix(verticalLerp11.x, verticalLerp11.y, fraction.y);
	vec2 verticalLerp21 = mix(vec2(values1.z, values2.z), vec2(values1.w, values2.w), fraction.xx);
	float pcfResult21 = mix(verticalLerp21.x, verticalLerp21.y, fraction.y);

	vec2 verticalLerp02 = mix(vec2(values2.x, values3.x), vec2(values2.y, values3.y), fraction.xx);
	float pcfResult02 = mix(verticalLerp02.x, verticalLerp02.y, fraction.y);
	vec2 verticalLerp12 = mix(vec2(values2.y, values3.y), vec2(values2.z, values3.z), fraction.xx);
	float pcfResult12 = mix(verticalLerp12.x, verticalLerp12.y, fraction.y);
	vec2 verticalLerp22 = mix(vec2(values2.z, values3.z), vec2(values2.w, values3.w), fraction.xx);
	float pcfResult22 = mix(verticalLerp22.x, verticalLerp22.y, fraction.y);

	float inShadow = (pcfResult00 + pcfResult10 + pcfResult20 + pcfResult01 + pcfResult11 + pcfResult21 + pcfResult02 + pcfResult12 + pcfResult22) * .11111;
	return inShadow;
}

float Pcf_2x2(sampler2D shadowMap, vec2 uv, float z0)
{
	vec2 vInvShadowMapWH = vec2(1.0) / vec2(textureSize(shadowMap, 0));
	vec2 fc = fract(uv / vInvShadowMapWH - 0.5);
	vec2 tc0 = uv - (fc - vec2(0.5)) * vInvShadowMapWH;
	vec2 tc1 = tc0 + vec2(vInvShadowMapWH.x, 0);
	vec2 tc2 = tc0 + vec2(0, vInvShadowMapWH.y);
	vec2 tc3 = tc1 + vec2(0, vInvShadowMapWH.y);

	vec4 litZ = vec4(z0);

	vec4 sampleDepth;
	sampleDepth.x = BorderDepthTexture(shadowMap, tc0);
	sampleDepth.y = BorderDepthTexture(shadowMap, tc1);
	sampleDepth.z = BorderDepthTexture(shadowMap, tc2);
	sampleDepth.w = BorderDepthTexture(shadowMap, tc3);

	vec4 inShadow4;
	inShadow4.x  = float(litZ.x > sampleDepth.x);
	inShadow4.y  = float(litZ.x > sampleDepth.y);
	inShadow4.z  = float(litZ.x > sampleDepth.z);
	inShadow4.w  = float(litZ.x > sampleDepth.w);

	float inShadow01 = mix(inShadow4.x, inShadow4.y, fc.x);
	float inShadow23 = mix(inShadow4.z, inShadow4.w, fc.x);
	float inShadow = mix(inShadow01, inShadow23, fc.y);
	return inShadow;
}

// Performs PCF filtering on the shadow map using multiple taps in the filter region.
float PcfFilter(sampler2D shadowMap, vec2 uv, float z0, vec2 dz_duv, vec2 filterRadiusUV)
{
	float sum = 0.0;
	for (int i = 0; i < 16; ++i)
	{
		vec2 offset = poisson16[i] * filterRadiusUV;
		float z = BiasedZ(z0, dz_duv, offset);
		sum += Pcf_2x2(shadowMap, uv + offset, z);
	}
	return sum / 16.0;
}

float PcssShadow(sampler2D shadowMap,
	vec2 lightRadiusUV,
	float lightZNear, float lightZFar,
	vec2 uv, float z, vec2 dz_duv, float zEye)
{
	// ------------------------
	// STEP 1: blocker search
	// ------------------------
	float accumBlockerDepth, numBlockers, maxBlockers;
	vec2 searchRegionRadiusUV = SearchRegionRadiusUV(lightRadiusUV, lightZNear, zEye);
	FindBlocker(accumBlockerDepth, numBlockers, maxBlockers, shadowMap, uv, z, dz_duv, searchRegionRadiusUV);

	// Early out if not in the penumbra
	if (numBlockers == 0.0)
		return 0.0;

	// ------------------------
	// STEP 2: penumbra size
	// ------------------------
	float avgBlockerDepth = accumBlockerDepth / numBlockers;
	float avgBlockerDepthWorld = ZClipToEye(lightZNear, lightZFar, avgBlockerDepth);
	vec2 penumbraRadius = PenumbraRadiusUV(lightRadiusUV, zEye, avgBlockerDepthWorld);
	vec2 filterRadius = ProjectToLightUV(lightZNear, penumbraRadius, zEye);

	// ------------------------
	// STEP 3: filtering
	// ------------------------
	return PcfFilter(shadowMap, uv, z, dz_duv, filterRadius);
}

float PcfShadow(sampler2D shadowMap,
	vec2 lightRadiusUV,
	float lightZNear, float lightZFar,
	vec2 uv, float z, vec2 dz_duv, float zEye)
{
	vec2 filterRadius = 0.1 * lightRadiusUV;
	return PcfFilter(shadowMap, uv, z, dz_duv, filterRadius);
}

const bool pcss_shadow = false;
const bool light_size_shadow = false;
const bool debug_layer = false;

float HardShadow(sampler2D shadowMap, vec2 uv, float z)
{
	float sampleDepth = BorderDepthTexture(shadowMap, uv);
	return float(z >= sampleDepth);
}

float SoftShadow(sampler2D shadowMap,
	vec2 lightRadiusUV,
	float lightZNear, float lightZFar,
	vec2 uv, float z, vec2 dz_duv, float zEye)
{
	if (light_size_shadow)
	{
		return pcss_shadow ? PcssShadow(shadowMap,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye) : PcfShadow(shadowMap,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else
	{
		return Pcf_4x4(shadowMap, uv, z);
	}
}

vec4 BlendCSMColor(uint cascaded, vec4 inColor)
{
	vec4 outColor = inColor;
	switch(cascaded)
	{
		case 0 : 
			outColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
			break;
		case 1 : 
			outColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
			break;
		case 2 : 
			outColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
			break;
		case 3 : 
			outColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
			break;
	}
	return outColor;
}

#endif