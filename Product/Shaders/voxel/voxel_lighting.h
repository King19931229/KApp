#ifndef VOXEL_LIGHTING_H
#define VOXEL_LIGHTING_H

float Linstep(float low, float high, float value)
{
	return clamp((value - low) / (high - low), 0.0f, 1.0f);
}

float ReduceLightBleeding(float pMax, float Amount)  
{
	return Linstep(Amount, 1, pMax);  
}

vec2 WarpDepth(float depth)
{
	depth = 2.0f * depth - 1.0f;
	float pos = exp(voxel.miscs2[2] * depth);
	float neg = -exp(-voxel.miscs2[3] * depth);
	return vec2(pos, neg);
}

float Chebyshev(vec2 moments, float mean, float minVariance)
{
	if(mean <= moments.x)
	{
		return 1.0f;
	}
	else
	{
		float variance = moments.y - (moments.x * moments.x);
		variance = max(variance, minVariance);
		float d = mean - moments.x;
		float lit = variance / (variance + (d * d));
		return ReduceLightBleeding(lit, voxel.miscs3[0]);
	}
}

float Visibility(vec3 position)
{
	/*
	vec4 lsPos = lightViewProjection * vec4(position, 1.0f);
	// avoid arithmetic error
	if(lsPos.w == 0.0f) return 1.0f;
	// transform to ndc-space
	lsPos /= lsPos.w;
	// querry visibility
	vec4 moments = texture(shadowMap, lsPos.xy);
	// move to avoid acne
	vec2 wDepth = WarpDepth(lsPos.z - 0.0001f);
	// derivative of warping at depth
	vec2 depthScale = 0.0001f * exponents * wDepth;
	vec2 minVariance = depthScale * depthScale;
	// evsm mode 4 compares negative and positive
	float positive = Chebyshev(moments.xz, wDepth.x, minVariance.x);
	float negative = Chebyshev(moments.yw, wDepth.y, minVariance.y);
	// shadowing value
	return min(positive, negative);
	*/
	return 0.0;
}

vec4 CalculateDirectional(Light light, vec3 normal, vec3 position, vec3 albedo)
{
	float visibility = 1.0f;

	if(light.shadowingMethod == 1)
	{
		visibility = Visibility(position);
	}
	else if(light.shadowingMethod == 2)
	{
		vec3 voxelPos = WorldToVoxel(position);
		visibility = TraceShadow(voxelPos, light.direction, 1.0f);
	}

	if(visibility == 0.0f) return vec4(0.0f); 

	return vec4(BRDF(light, normal, albedo) * visibility, visibility);
}

vec4 CalculatePoint(Light light, vec3 normal, vec3 position, vec3 albedo)
{
	light.direction = light.position - position;
	float d = length(light.direction);
	light.direction = normalize(light.direction);
	float falloff = 1.0f / (light.attenuation.constant + light.attenuation.linear * d
					+ light.attenuation.quadratic * d * d + 1.0f);

	if(falloff <= 0.0f) return vec4(0.0f);

	float visibility = 1.0f;

	if(light.shadowingMethod == 2)
	{
		vec3 voxelPos = WorldToVoxel(position);
		vec3 lightPosT = WorldToVoxel(light.position);

		vec3 lightDirT = lightPosT.xyz - voxelPos.xyz;
		float dT = length(lightDirT);
		lightDirT = normalize(lightDirT);

		visibility = TraceShadow(voxelPos, lightDirT, dT);
	}

	if(visibility <= 0.0f) return vec4(0.0f); 

	return vec4(BRDF(light, normal, albedo) * falloff * visibility, visibility);
}

vec4 CalculateSpot(Light light, vec3 normal, vec3 position, vec3 albedo)
{
	vec3 spotDirection = light.direction;
	light.direction = normalize(light.position - position);
	float cosAngle = dot(-light.direction, spotDirection);

	// outside the cone
	if(cosAngle < light.angleOuterCone) { return vec4(0.0f); }

	// assuming they are passed as cos(angle)
	float innerMinusOuter = light.angleInnerCone - light.angleOuterCone;
	// spot light factor for smooth transition
	float spotMark = (cosAngle - light.angleOuterCone) / innerMinusOuter;
	float spotFalloff = smoothstep(0.0f, 1.0f, spotMark);

	if(spotFalloff <= 0.0f) return vec4(0.0f);   

	float dst = distance(light.position, position);
	float falloff = 1.0f / (light.attenuation.constant + light.attenuation.linear * dst
					+ light.attenuation.quadratic * dst * dst + 1.0f);   

	if(falloff <= 0.0f) return vec4(0.0f);

	float visibility = 1.0f;

	if(light.shadowingMethod == 2)
	{
		vec3 voxelPos = WorldToVoxel(position);
		vec3 lightPosT = WorldToVoxel(light.position);

		vec3 lightDirT = lightPosT.xyz - voxelPos.xyz;
		float dT = length(lightDirT);
		lightDirT = normalize(lightDirT);

		visibility = TraceShadow(voxelPos, lightDirT, dT);
	}

	if(visibility <= 0.0f) return vec4(0.0f);

	return vec4(BRDF(light, normal, albedo) * falloff * spotFalloff * visibility, visibility);
}

vec4 CalculateDirectLighting(vec3 position, vec3 normal, vec3 albedo)
{
	normal = normalize(normal);
	// world space grid voxel size
	float voxelWorldSize = 1.0 /  (voxel.minpoint_scale.w * voxel.miscs[0]);
	// calculate directional lighting
	vec4 directLighting = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	vec4 current =  vec4(0.0f);
	int count = 0;
	// move position forward to avoid shadowing errors
	position = position + normal * voxelWorldSize * 0.5f;

	// calculate lighting for sun light
	{
		Light sunLight;
		sunLight.diffuse = vec3(1.0);
		sunLight.direction = voxel.sunlight.xyz;
		sunLight.shadowingMethod = 2;
		directLighting = CalculateDirectional(sunLight, normal, position, albedo);
		++count;
	}

#if 0
	// calculate lighting for directional lights
	for(int i = 0; i < lightTypeCount[0]; ++i)
	{
		current = CalculateDirectional(directionalLight[i], normal, position, albedo);
		directLighting.rgb += current.rgb;
		directLighting.a += current.a; count++;
	}

	// calculate lighting for point lights
	for(int i = 0; i < lightTypeCount[1]; ++i)
	{
		current = CalculatePoint(pointLight[i], normal, position, albedo);
		directLighting.rgb += current.rgb;
		directLighting.a += current.a; count++;
	}

	// calculate lighting for spot lights
	for(int i = 0; i < lightTypeCount[2]; ++i) 
	{
		current = CalculateSpot(spotLight[i], normal, position, albedo);
		directLighting.rgb += current.rgb;
		directLighting.a += current.a; count++;
	}
#endif

	if(count > 0) { directLighting.a /= count; }

	return directLighting;
}

#endif