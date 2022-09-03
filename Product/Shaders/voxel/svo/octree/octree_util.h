#ifndef OCTREE_UTIL_H
#define OCTREE_UTIL_H
/*
 *  Copyright (c) 2009-2011, NVIDIA Corporation
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of NVIDIA Corporation nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define STACK_SIZE 23
#define EPS 3.552713678800501e-15

bool GetOctreeIndex(in uvec3 level_pos, in uint level_dim, out uint idx)
{
	uint cur = 0u;
	bvec3 level_cmp;

	idx = 0u;
	do
	{
		level_dim >>= 1;
		level_cmp = greaterThanEqual(level_pos, uvec3(level_dim));
		idx = cur + (uint(level_cmp.x) | (uint(level_cmp.y) << 1u) | (uint(level_cmp.z) << 2u));
		cur = uOctree[idx] & 0x3fffffffu;
		level_pos -= uvec3(level_cmp) * level_dim;
	} while (cur != 0u && level_dim > 1u);

	return level_dim == 1;
}

bool GetOctreeNodeIndex(in uvec3 level_pos, in uint level_dim, in uint node_size, out uint idx)
{
	uint cur = 0u;
	bvec3 level_cmp;

	idx = 0u;
	while (level_dim > node_size)
	{
		level_dim >>= 1;
		level_cmp = greaterThanEqual(level_pos, uvec3(level_dim));
		idx = cur + (uint(level_cmp.x) | (uint(level_cmp.y) << 1u) | (uint(level_cmp.z) << 2u));
		cur = uOctree[idx] & 0x3fffffffu;
		level_pos -= uvec3(level_cmp) * level_dim;
		if (cur == 0u)
			break;
	}

	return level_dim == node_size;
}

bool GetOctreeDataIndex(in uvec3 level_pos, in uint level_dim, out uint idx)
{
	if (GetOctreeIndex(level_pos, level_dim, idx))
	{
		idx = uOctree[idx] & 0x3fffffffu;
		return true;
	}
	return false;
}

bool InsideBound(in uint level_dim, ivec3 pos)
{
	if (pos.x < 0 || pos.y < 0 || pos.z < 0 || pos.x >= level_dim || pos.y >= level_dim || pos.z >= level_dim)
		return false;
	return true;
}

bool StoreOctreeRadiance(in uint level_dim, ivec3 store_pos, in vec4 data)
{
	uint idx = 0;
	if (InsideBound(level_dim, store_pos) && GetOctreeDataIndex(uvec3(store_pos), level_dim, idx))
	{
		uint encoded = packUnorm4x8(data);
		uOctreeData[idx][OCTREE_RADIANCE_INDEX] = encoded;
		return true;
	}
	return false;
}

bool ComputeWeightsMipmap(in uint level_dim, in uint mipmap, in vec3 uvw, out bool valids[8], out uint idxs[8], out float weights[8])
{
	int node_size = 1 << int(mipmap + 1);
	level_dim /= node_size;
	vec3 pos = float(level_dim) * uvw - vec3(0.5);
	vec3 floor_pos = floor(pos);
	ivec3 base_pos = ivec3(floor_pos);
	vec3 texel_pos = pos - floor_pos;

	base_pos *= node_size;
	level_dim *= node_size;
	ivec3 sample_pos[8];
	sample_pos[0] = base_pos + node_size * ivec3(0, 0, 0);
	sample_pos[1] = base_pos + node_size * ivec3(1, 0, 0);
	sample_pos[2] = base_pos + node_size * ivec3(0, 1, 0);
	sample_pos[3] = base_pos + node_size * ivec3(1, 1, 0);
	sample_pos[4] = base_pos + node_size * ivec3(0, 0, 1);
	sample_pos[5] = base_pos + node_size * ivec3(1, 0, 1);
	sample_pos[6] = base_pos + node_size * ivec3(0, 1, 1);
	sample_pos[7] = base_pos + node_size * ivec3(1, 1, 1);

	weights[0] = 1.0 - texel_pos.x;
	weights[1] = texel_pos.x;
	weights[2] = 1.0 - texel_pos.x;
	weights[3] = texel_pos.x;
	weights[4] = 1.0 - texel_pos.x;
	weights[5] = texel_pos.x;
	weights[6] = 1.0 - texel_pos.x;
	weights[7] = texel_pos.x;

	weights[0] *= 1.0 - texel_pos.y;
	weights[1] *= 1.0 - texel_pos.y;
	weights[2] *= texel_pos.y;
	weights[3] *= texel_pos.y;
	weights[4] *= 1.0 - texel_pos.y;
	weights[5] *= 1.0 - texel_pos.y;
	weights[6] *= texel_pos.y;
	weights[7] *= texel_pos.y;

	weights[0] *= 1.0 - texel_pos.z;
	weights[1] *= 1.0 - texel_pos.z;
	weights[2] *= 1.0 - texel_pos.z;
	weights[3] *= 1.0 - texel_pos.z;
	weights[4] *= texel_pos.z;
	weights[5] *= texel_pos.z;
	weights[6] *= texel_pos.z;
	weights[7] *= texel_pos.z;

	float sum = 0.0;
	for(int i = 0; i < 8; ++i)
	{
		valids[i] = InsideBound(level_dim, sample_pos[i]);
		if(valids[i])
		{
			valids[i] = GetOctreeNodeIndex(uvec3(sample_pos[i]), level_dim, node_size, idxs[i]);
		}
		sum += weights[i];
	}

	for(int i = 0; i < 8; ++i)
	{
		weights[i] /= sum;
	}

	return abs(sum - 1.0) < 1e-5;
}

bool ComputeClosestMipmap(in uint level_dim, in uint mipmap, in vec3 uvw, out uint idx)
{
	int node_size = 1 << int(mipmap + 1);
	level_dim /= node_size;
	vec3 pos = float(level_dim) * uvw - vec3(0.5);
	vec3 floor_pos = floor(pos);
	ivec3 base_pos = ivec3(floor_pos);
	vec3 texel_pos = pos - floor_pos;
	texel_pos = round(texel_pos);

	base_pos *= node_size;
	level_dim *= node_size;

	ivec3 sample_pos = base_pos + node_size * ivec3(texel_pos);
	if (InsideBound(level_dim, sample_pos) && GetOctreeNodeIndex(uvec3(sample_pos), level_dim, node_size, idx))
	{
		return true;
	}
	return false;
}

bool ComputeWeights(in uint level_dim, in vec3 uvw, out bool valids[8], out uint idxs[8], out float weights[8])
{
	vec3 pos = float(level_dim) * uvw - vec3(0.5);
	vec3 floor_pos = floor(pos);
	ivec3 base_pos = ivec3(floor_pos);
	vec3 texel_pos = pos - floor_pos;

	ivec3 sample_pos[8];
	sample_pos[0] = base_pos + ivec3(0, 0, 0);
	sample_pos[1] = base_pos + ivec3(1, 0, 0);
	sample_pos[2] = base_pos + ivec3(0, 1, 0);
	sample_pos[3] = base_pos + ivec3(1, 1, 0);
	sample_pos[4] = base_pos + ivec3(0, 0, 1);
	sample_pos[5] = base_pos + ivec3(1, 0, 1);
	sample_pos[6] = base_pos + ivec3(0, 1, 1);
	sample_pos[7] = base_pos + ivec3(1, 1, 1);

	weights[0] = 1.0 - texel_pos.x;
	weights[1] = texel_pos.x;
	weights[2] = 1.0 - texel_pos.x;
	weights[3] = texel_pos.x;
	weights[4] = 1.0 - texel_pos.x;
	weights[5] = texel_pos.x;
	weights[6] = 1.0 - texel_pos.x;
	weights[7] = texel_pos.x;

	weights[0] *= 1.0 - texel_pos.y;
	weights[1] *= 1.0 - texel_pos.y;
	weights[2] *= texel_pos.y;
	weights[3] *= texel_pos.y;
	weights[4] *= 1.0 - texel_pos.y;
	weights[5] *= 1.0 - texel_pos.y;
	weights[6] *= texel_pos.y;
	weights[7] *= texel_pos.y;

	weights[0] *= 1.0 - texel_pos.z;
	weights[1] *= 1.0 - texel_pos.z;
	weights[2] *= 1.0 - texel_pos.z;
	weights[3] *= 1.0 - texel_pos.z;
	weights[4] *= texel_pos.z;
	weights[5] *= texel_pos.z;
	weights[6] *= texel_pos.z;
	weights[7] *= texel_pos.z;

	float sum = 0.0;
	for(int i = 0; i < 8; ++i)
	{
		valids[i] = InsideBound(level_dim, sample_pos[i]);
		if(valids[i])
		{
			valids[i] = GetOctreeDataIndex(uvec3(sample_pos[i]), level_dim, idxs[i]);
		}
		sum += weights[i];
	}

	for(int i = 0; i < 8; ++i)
	{
		weights[i] /= sum;
	}

	return abs(sum - 1.0) < 1e-5;
}

vec4 SampleOctreeMipmapDataSingleLevel(in uint level_dim, in uint mipmap, in vec3 uvw, in uint dir_index)
{
	bool valids[8];
	uint idxs[8];
	float weights[8];

	vec4 data = vec4(0);

	if (ComputeWeightsMipmap(level_dim, mipmap, uvw, valids, idxs, weights))
	{
		for(int i = 0; i < 8; ++i)
		{
			if (valids[i])
			{
				uint idx = idxs[i];
				vec4 unpacked = unpackUnorm4x8(uOctreeMipmapData[idx][dir_index]);
				data += weights[i] * unpacked;
			}
		}
	}

	return data;
}

vec4 SampleOctreeMipmapDataSingleLevelClosest(in uint level_dim, in uint mipmap, in vec3 uvw, in uint dir_index)
{
	uint idx = 0;
	vec4 data = vec4(0);
	if (ComputeClosestMipmap(level_dim, mipmap, uvw, idx))
	{
		data = unpackUnorm4x8(uOctreeMipmapData[idx][dir_index]);
	}
	return data;
}

vec4 SampleOctreeMipmap(in uint level_dim, in vec3 uvw, float mipmap, in uint dir_index)
{
	const uint max_mipmap = uint(log2(level_dim) - 1);
	uint floor_mipmap = uint(floor(mipmap));
	uint ceil_mipmap = floor_mipmap + 1;
	floor_mipmap = max(floor_mipmap, 0);
	floor_mipmap = min(floor_mipmap, max_mipmap);
	ceil_mipmap = max(ceil_mipmap, 0);
	ceil_mipmap = min(ceil_mipmap, max_mipmap);
	float factor = max(mipmap - float(floor_mipmap), 0);
	return mix(SampleOctreeMipmapDataSingleLevel(level_dim, floor_mipmap, uvw, dir_index),
		SampleOctreeMipmapDataSingleLevel(level_dim, ceil_mipmap, uvw, dir_index),
		factor);
}

vec4 SampleOctreeMipmapClosest(in uint level_dim, in vec3 uvw, float mipmap, in uint dir_index)
{
	const uint max_mipmap = uint(log2(level_dim) - 1);
	uint floor_mipmap = uint(floor(mipmap));
	uint ceil_mipmap = floor_mipmap + 1;
	floor_mipmap = max(floor_mipmap, 0);
	floor_mipmap = min(floor_mipmap, max_mipmap);
	ceil_mipmap = max(ceil_mipmap, 0);
	ceil_mipmap = min(ceil_mipmap, max_mipmap);
	float factor = max(mipmap - float(floor_mipmap), 0);
	factor = round(factor);
	if (factor == 0)
	{
		return SampleOctreeMipmapDataSingleLevelClosest(level_dim, floor_mipmap, uvw, dir_index);
	}
	else
	{
		return SampleOctreeMipmapDataSingleLevelClosest(level_dim, ceil_mipmap, uvw, dir_index);
	}
}

vec4 SampleOctreeSingleData(in uint level_dim, in vec3 uvw, in uint data_index)
{
	bool valids[8];
	uint idxs[8];
	float weights[8];

	vec4 data = vec4(0);

	if (ComputeWeights(level_dim, uvw, valids, idxs, weights))
	{
		for(int i = 0; i < 8; ++i)
		{
			if (valids[i])
			{
				uint idx = idxs[i];
				vec4 unpacked = unpackUnorm4x8(uOctreeData[idx][data_index]);
				uint count = 0;
				if (data_index != OCTREE_RADIANCE_INDEX)
				{
					count = uint(unpacked.w * 255.0);
					count = (count & 0x3fu) > 0 ? 1 : 0;
				}
				else
				{
					vec4 col_unpacked = unpackUnorm4x8(uOctreeData[idx][OCTREE_COLOR_INDEX]);
					count = col_unpacked.w > 0 ? 1 : 0;
				}
				data += weights[i] * vec4(unpacked.xyz, count);
			}
		}
	}

	return data;
}

vec4 SampleOctreeColor(in uint level_dim, in vec3 uvw)
{
	return SampleOctreeSingleData(level_dim, uvw, OCTREE_COLOR_INDEX);
}

vec4 SampleOctreeNormal(in uint level_dim, in vec3 uvw)
{
	return SampleOctreeSingleData(level_dim, uvw, OCTREE_NORMAL_INDEX);
}

vec4 SampleOctreeEmssive(in uint level_dim, in vec3 uvw)
{
	return SampleOctreeSingleData(level_dim, uvw, OCTREE_EMISSIVE_INDEX);
}

vec4 SampleOctreeRadiance(in uint level_dim, in vec3 uvw)
{
	return SampleOctreeSingleData(level_dim, uvw, OCTREE_RADIANCE_INDEX);
}

float SampleOctreeVisibility(in uint level_dim, in vec3 uvw)
{
	bool valids[8];
	uint idxs[8];
	float weights[8];

	float data = 0;

	if (ComputeWeights(level_dim, uvw, valids, idxs, weights))
	{
		for(int i = 0; i < 8; ++i)
		{
			if (valids[i])
			{
				uint idx = idxs[i];
				vec4 unpacked = unpackUnorm4x8(uOctreeData[idx][OCTREE_RADIANCE_INDEX]);
				data += weights[i] * unpacked.w;
			}
		}
	}

	return data;
}

bool SampleOctree(in uint level_dim, in vec3 uvw, out vec4 o_color, out vec4 o_normal, out vec4 o_emissive)
{
	bool valids[8];
	uint idxs[8];
	float weights[8];

	vec4 color = vec4(0);
	vec4 norm = vec4(0);
	vec4 emissive = vec4(0);

	if(ComputeWeights(level_dim, uvw, valids, idxs, weights))
	{
		for(int i = 0; i < 8; ++i)
		{
			if(valids[i])
			{
				uint idx = idxs[i];

				vec4 unpacked = vec4(0);
				uint count = 0;

				unpacked = unpackUnorm4x8(uOctreeData[idx][OCTREE_COLOR_INDEX]);
				count = uint(unpacked.w * 255.0);
				count = count & 0x3fu;
				color += weights[i] * vec4(unpacked.xyz, count > 0 ? 1 : 0);

				unpacked = unpackUnorm4x8(uOctreeData[idx][OCTREE_NORMAL_INDEX]);
				count = uint(unpacked.w * 255.0);
				count = count & 0x3fu;
				norm += weights[i] * vec4(unpacked.xyz, count > 0 ? 1 : 0);

				unpacked = unpackUnorm4x8(uOctreeData[idx][OCTREE_EMISSIVE_INDEX]);
				count = uint(unpacked.w * 255.0);
				count = count & 0x3fu;
				emissive += weights[i] * vec4(unpacked.xyz, count > 0 ? 1 : 0);
			}
		}
	}

	o_color = color;
	o_normal = norm;
	o_emissive = emissive;

	return true;
}

struct StackItem {
	uint node;
	float t_max;
} stack[STACK_SIZE];

bool RayMarchLeaf(vec3 o, vec3 d, out float o_t, out vec3 o_color, out vec3 o_normal, out vec3 o_emissive, out vec3 o_radiance, out uint o_iter) {
	uint iter = 0;

	d.x = abs(d.x) > EPS ? d.x : (d.x >= 0 ? EPS : -EPS);
	d.y = abs(d.y) > EPS ? d.y : (d.y >= 0 ? EPS : -EPS);
	d.z = abs(d.z) > EPS ? d.z : (d.z >= 0 ? EPS : -EPS);

	// Precompute the coefficients of tx(x), ty(y), and tz(z).
	// The octree is assumed to reside at coordinates [1, 2].
	vec3 t_coef = 1.0f / -abs(d);
	vec3 t_bias = t_coef * o;

	uint oct_mask = 0u;
	if (d.x > 0.0f)
		oct_mask ^= 1u, t_bias.x = 3.0f * t_coef.x - t_bias.x;
	if (d.y > 0.0f)
		oct_mask ^= 2u, t_bias.y = 3.0f * t_coef.y - t_bias.y;
	if (d.z > 0.0f)
		oct_mask ^= 4u, t_bias.z = 3.0f * t_coef.z - t_bias.z;

	// Initialize the active span of t-values.
	float t_min = max(max(2.0f * t_coef.x - t_bias.x, 2.0f * t_coef.y - t_bias.y), 2.0f * t_coef.z - t_bias.z);
	float t_max = min(min(t_coef.x - t_bias.x, t_coef.y - t_bias.y), t_coef.z - t_bias.z);
	t_min = max(t_min, 0.0f);
	float h = t_max;

	uint parent = 0u;
	uint cur = 0;
	vec3 pos = vec3(1.0f);
	uint idx = 0u;
	if (1.5f * t_coef.x - t_bias.x > t_min)
		idx ^= 1u, pos.x = 1.5f;
	if (1.5f * t_coef.y - t_bias.y > t_min)
		idx ^= 2u, pos.y = 1.5f;
	if (1.5f * t_coef.z - t_bias.z > t_min)
		idx ^= 4u, pos.z = 1.5f;

	uint scale = STACK_SIZE - 1;
	float scale_exp2 = 0.5f; // exp2( scale - STACK_SIZE )

	while (scale < STACK_SIZE) {
		++iter;
		if (cur == 0u)
			cur = uOctree[parent + (idx ^ oct_mask)];
		// Determine maximum t-value of the cube by evaluating
		// tx(), ty(), and tz() at its corner.

		vec3 t_corner = pos * t_coef - t_bias;
		float tc_max = min(min(t_corner.x, t_corner.y), t_corner.z);

		if ((cur & 0x80000000u) != 0 && t_min <= t_max) {
			// INTERSECT
			float tv_max = min(t_max, tc_max);
			float half_scale_exp2 = scale_exp2 * 0.5f;
			vec3 t_center = half_scale_exp2 * t_coef + t_corner;

			if (t_min <= tv_max) {
				if ((cur & 0x40000000u) != 0) // leaf node
					break;

				// PUSH
				if (tc_max < h) {
					stack[scale].node = parent;
					stack[scale].t_max = t_max;
				}
				h = tc_max;

				parent = cur & 0x3fffffffu;

				idx = 0u;
				--scale;
				scale_exp2 = half_scale_exp2;
				if (t_center.x > t_min)
					idx ^= 1u, pos.x += scale_exp2;
				if (t_center.y > t_min)
					idx ^= 2u, pos.y += scale_exp2;
				if (t_center.z > t_min)
					idx ^= 4u, pos.z += scale_exp2;

				cur = 0;
				t_max = tv_max;

				continue;
			}
		}

		// ADVANCE
		uint step_mask = 0u;
		if (t_corner.x <= tc_max)
			step_mask ^= 1u, pos.x -= scale_exp2;
		if (t_corner.y <= tc_max)
			step_mask ^= 2u, pos.y -= scale_exp2;
		if (t_corner.z <= tc_max)
			step_mask ^= 4u, pos.z -= scale_exp2;

		// Update active t-span and flip bits of the child slot index.
		t_min = tc_max;
		idx ^= step_mask;

		// Proceed with pop if the bit flips disagree with the ray direction.
		if ((idx & step_mask) != 0) {
			// POP
			// Find the highest differing bit between the two positions.
			uint differing_bits = 0;
			if ((step_mask & 1u) != 0)
				differing_bits |= floatBitsToUint(pos.x) ^ floatBitsToUint(pos.x + scale_exp2);
			if ((step_mask & 2u) != 0)
				differing_bits |= floatBitsToUint(pos.y) ^ floatBitsToUint(pos.y + scale_exp2);
			if ((step_mask & 4u) != 0)
				differing_bits |= floatBitsToUint(pos.z) ^ floatBitsToUint(pos.z + scale_exp2);
			scale = findMSB(differing_bits);
			scale_exp2 = uintBitsToFloat((scale - STACK_SIZE + 127u) << 23u); // exp2f(scale - s_max)

			// Restore parent voxel from the stack.
			parent = stack[scale].node;
			t_max = stack[scale].t_max;

			// Round cube position and extract child slot index.
			uint shx = floatBitsToUint(pos.x) >> scale;
			uint shy = floatBitsToUint(pos.y) >> scale;
			uint shz = floatBitsToUint(pos.z) >> scale;
			pos.x = uintBitsToFloat(shx << scale);
			pos.y = uintBitsToFloat(shy << scale);
			pos.z = uintBitsToFloat(shz << scale);
			idx = (shx & 1u) | ((shy & 1u) << 1u) | ((shz & 1u) << 2u);

			// Prevent same parent from being stored again and invalidate cached
			// child descriptor.
			h = 0.0f;
			cur = 0;
		}
	}

	uint data_idx = cur & 0x3fffffffu;
#if 0
	vec3 t_corner = t_coef * (pos + scale_exp2) - t_bias;

	vec3 norm = (t_corner.x > t_corner.y && t_corner.x > t_corner.z)
					? vec3(-1, 0, 0)
					: (t_corner.y > t_corner.z ? vec3(0, -1, 0) : vec3(0, 0, -1));
	if ((oct_mask & 1u) == 0u)
		norm.x = -norm.x;
	if ((oct_mask & 2u) == 0u)
		norm.y = -norm.y;
	if ((oct_mask & 4u) == 0u)
		norm.z = -norm.z;

	vec3 emissive = vec3(0);
	vec3 radiance = vec3(0);
#else
	vec3 norm = 2.0 * unpackUnorm4x8(uOctreeData[data_idx][OCTREE_NORMAL_INDEX]).xyz - vec3(1.0);
	vec3 emissive = unpackUnorm4x8(uOctreeData[data_idx][OCTREE_EMISSIVE_INDEX]).xyz;
	vec3 radiance = unpackUnorm4x8(uOctreeData[data_idx][OCTREE_RADIANCE_INDEX]).xyz;
#endif
	vec3 color = unpackUnorm4x8(uOctreeData[data_idx][OCTREE_COLOR_INDEX]).xyz;

	o_normal = norm;
	o_emissive = emissive;
	o_radiance = radiance;
	o_color = color;
	o_t = t_min;
	o_iter = iter;

	return scale < STACK_SIZE && t_min <= t_max;
}

bool RayMarchCoarse(vec3 o, vec3 d, float orig_sz, float dir_sz, out float t, out float size) {
	d.x = abs(d.x) > EPS ? d.x : (d.x >= 0 ? EPS : -EPS);
	d.y = abs(d.y) > EPS ? d.y : (d.y >= 0 ? EPS : -EPS);
	d.z = abs(d.z) > EPS ? d.z : (d.z >= 0 ? EPS : -EPS);

	// Precompute the coefficients of tx(x), ty(y), and tz(z).
	// The octree is assumed to reside at coordinates [1, 2].
	vec3 t_coef = 1.0f / -abs(d);
	vec3 t_bias = t_coef * o;

	uint oct_mask = 0u;
	if (d.x > 0.0f)
		oct_mask ^= 1u, t_bias.x = 3.0f * t_coef.x - t_bias.x;
	if (d.y > 0.0f)
		oct_mask ^= 2u, t_bias.y = 3.0f * t_coef.y - t_bias.y;
	if (d.z > 0.0f)
		oct_mask ^= 4u, t_bias.z = 3.0f * t_coef.z - t_bias.z;

	// Initialize the active span of t-values.
	float t_min = max(max(2.0f * t_coef.x - t_bias.x, 2.0f * t_coef.y - t_bias.y), 2.0f * t_coef.z - t_bias.z);
	float t_max = min(min(t_coef.x - t_bias.x, t_coef.y - t_bias.y), t_coef.z - t_bias.z);
	t_min = max(t_min, 0.0f);
	float h = t_max;

	uint parent = 0u;
	uint cur = 0u;
	vec3 pos = vec3(1.0f);
	uint idx = 0u;
	if (1.5f * t_coef.x - t_bias.x > t_min)
		idx ^= 1u, pos.x = 1.5f;
	if (1.5f * t_coef.y - t_bias.y > t_min)
		idx ^= 2u, pos.y = 1.5f;
	if (1.5f * t_coef.z - t_bias.z > t_min)
		idx ^= 4u, pos.z = 1.5f;

	uint scale = STACK_SIZE - 1;
	float scale_exp2 = 0.5f; // exp2( scale - STACK_SIZE )

	while (scale < STACK_SIZE) {
		if (cur == 0u)
			cur = uOctree[parent + (idx ^ oct_mask)];
		// Determine maximum t-value of the cube by evaluating
		// tx(), ty(), and tz() at its corner.

		vec3 t_corner = pos * t_coef - t_bias;
		float tc_max = min(min(t_corner.x, t_corner.y), t_corner.z);

		if ((cur & 0x80000000u) != 0 && t_min <= t_max) {
			if (orig_sz + tc_max * dir_sz >= scale_exp2)
				break;

			// INTERSECT
			float tv_max = min(t_max, tc_max);
			float half_scale_exp2 = scale_exp2 * 0.5f;
			vec3 t_center = half_scale_exp2 * t_coef + t_corner;

			if (t_min <= tv_max) {
				if ((cur & 0x40000000u) != 0) // leaf node
					break;
				// PUSH
				if (tc_max < h) {
					stack[scale].node = parent;
					stack[scale].t_max = t_max;
				}
				h = tc_max;

				parent = cur & 0x3fffffffu;

				idx = 0u;
				--scale;
				scale_exp2 = half_scale_exp2;
				if (t_center.x > t_min)
					idx ^= 1u, pos.x += scale_exp2;
				if (t_center.y > t_min)
					idx ^= 2u, pos.y += scale_exp2;
				if (t_center.z > t_min)
					idx ^= 4u, pos.z += scale_exp2;

				cur = 0;
				t_max = tv_max;

				continue;
			}
		}

		// ADVANCE
		uint step_mask = 0u;
		if (t_corner.x <= tc_max)
			step_mask ^= 1u, pos.x -= scale_exp2;
		if (t_corner.y <= tc_max)
			step_mask ^= 2u, pos.y -= scale_exp2;
		if (t_corner.z <= tc_max)
			step_mask ^= 4u, pos.z -= scale_exp2;

		// Update active t-span and flip bits of the child slot index.
		t_min = tc_max;
		idx ^= step_mask;

		// Proceed with pop if the bit flips disagree with the ray direction.
		if ((idx & step_mask) != 0) {
			// POP
			// Find the highest differing bit between the two positions.
			uint differing_bits = 0;
			if ((step_mask & 1u) != 0)
				differing_bits |= floatBitsToUint(pos.x) ^ floatBitsToUint(pos.x + scale_exp2);
			if ((step_mask & 2u) != 0)
				differing_bits |= floatBitsToUint(pos.y) ^ floatBitsToUint(pos.y + scale_exp2);
			if ((step_mask & 4u) != 0)
				differing_bits |= floatBitsToUint(pos.z) ^ floatBitsToUint(pos.z + scale_exp2);
			scale = findMSB(differing_bits);
			scale_exp2 = uintBitsToFloat((scale - STACK_SIZE + 127u) << 23u); // exp2f(scale - s_max)

			// Restore parent voxel from the stack.
			parent = stack[scale].node;
			t_max = stack[scale].t_max;

			// Round cube position and extract child slot index.
			uint shx = floatBitsToUint(pos.x) >> scale;
			uint shy = floatBitsToUint(pos.y) >> scale;
			uint shz = floatBitsToUint(pos.z) >> scale;
			pos.x = uintBitsToFloat(shx << scale);
			pos.y = uintBitsToFloat(shy << scale);
			pos.z = uintBitsToFloat(shz << scale);
			idx = (shx & 1u) | ((shy & 1u) << 1u) | ((shz & 1u) << 2u);

			// Prevent same parent from being stored again and invalidate cached
			// child descriptor.
			h = 0.0f;
			cur = 0;
		}
	}
	t = t_min;
	size = scale_exp2;
	return scale < STACK_SIZE && t_min <= t_max;
}

bool RayMarchOcclude(vec3 o, vec3 d) {
	d.x = abs(d.x) >= EPS ? d.x : (d.x >= 0 ? EPS : -EPS);
	d.y = abs(d.y) >= EPS ? d.y : (d.y >= 0 ? EPS : -EPS);
	d.z = abs(d.z) >= EPS ? d.z : (d.z >= 0 ? EPS : -EPS);

	// Precompute the coefficients of tx(x), ty(y), and tz(z).
	// The octree is assumed to reside at coordinates [1, 2].
	vec3 t_coef = 1.0f / -abs(d);
	vec3 t_bias = t_coef * o;

	uint oct_mask = 0u;
	if (d.x > 0.0f)
		oct_mask ^= 1u, t_bias.x = 3.0f * t_coef.x - t_bias.x;
	if (d.y > 0.0f)
		oct_mask ^= 2u, t_bias.y = 3.0f * t_coef.y - t_bias.y;
	if (d.z > 0.0f)
		oct_mask ^= 4u, t_bias.z = 3.0f * t_coef.z - t_bias.z;

	// Initialize the active span of t-values.
	float t_min = max(max(2.0f * t_coef.x - t_bias.x, 2.0f * t_coef.y - t_bias.y), 2.0f * t_coef.z - t_bias.z);
	float t_max = min(min(t_coef.x - t_bias.x, t_coef.y - t_bias.y), t_coef.z - t_bias.z);
	t_min = max(t_min, 0.0f);
	float h = t_max;

	uint parent = 0u;
	uint cur = 0u;
	vec3 pos = vec3(1.0f);
	uint idx = 0u;
	if (1.5f * t_coef.x - t_bias.x > t_min)
		idx ^= 1u, pos.x = 1.5f;
	if (1.5f * t_coef.y - t_bias.y > t_min)
		idx ^= 2u, pos.y = 1.5f;
	if (1.5f * t_coef.z - t_bias.z > t_min)
		idx ^= 4u, pos.z = 1.5f;

	uint scale = STACK_SIZE - 1;
	float scale_exp2 = 0.5f; // exp2( scale - STACK_SIZE )

	while (scale < STACK_SIZE) {
		if (cur == 0u)
			cur = uOctree[parent + (idx ^ oct_mask)];
		// Determine maximum t-value of the cube by evaluating
		// tx(), ty(), and tz() at its corner.

		vec3 t_corner = pos * t_coef - t_bias;
		float tc_max = min(min(t_corner.x, t_corner.y), t_corner.z);

		if ((cur & 0x80000000u) != 0 && t_min <= t_max) {
			// INTERSECT
			float tv_max = min(t_max, tc_max);
			float half_scale_exp2 = scale_exp2 * 0.5f;
			vec3 t_center = half_scale_exp2 * t_coef + t_corner;

			if (t_min <= tv_max) {
				if ((cur & 0x40000000u) != 0) // leaf node
					break;

				// PUSH
				if (tc_max < h) {
					stack[scale].node = parent;
					stack[scale].t_max = t_max;
				}
				h = tc_max;

				parent = cur & 0x3fffffffu;

				idx = 0u;
				--scale;
				scale_exp2 = half_scale_exp2;
				if (t_center.x > t_min)
					idx ^= 1u, pos.x += scale_exp2;
				if (t_center.y > t_min)
					idx ^= 2u, pos.y += scale_exp2;
				if (t_center.z > t_min)
					idx ^= 4u, pos.z += scale_exp2;

				cur = 0;
				t_max = tv_max;

				continue;
			}
		}

		// ADVANCE
		uint step_mask = 0u;
		if (t_corner.x <= tc_max)
			step_mask ^= 1u, pos.x -= scale_exp2;
		if (t_corner.y <= tc_max)
			step_mask ^= 2u, pos.y -= scale_exp2;
		if (t_corner.z <= tc_max)
			step_mask ^= 4u, pos.z -= scale_exp2;

		// Update active t-span and flip bits of the child slot index.
		t_min = tc_max;
		idx ^= step_mask;

		// Proceed with pop if the bit flips disagree with the ray direction.
		if ((idx & step_mask) != 0) {
			// POP
			// Find the highest differing bit between the two positions.
			uint differing_bits = 0;
			if ((step_mask & 1u) != 0)
				differing_bits |= floatBitsToUint(pos.x) ^ floatBitsToUint(pos.x + scale_exp2);
			if ((step_mask & 2u) != 0)
				differing_bits |= floatBitsToUint(pos.y) ^ floatBitsToUint(pos.y + scale_exp2);
			if ((step_mask & 4u) != 0)
				differing_bits |= floatBitsToUint(pos.z) ^ floatBitsToUint(pos.z + scale_exp2);
			scale = findMSB(differing_bits);
			scale_exp2 = uintBitsToFloat((scale - STACK_SIZE + 127u) << 23u); // exp2f(scale - s_max)

			// Restore parent voxel from the stack.
			parent = stack[scale].node;
			t_max = stack[scale].t_max;

			// Round cube position and extract child slot index.
			uint shx = floatBitsToUint(pos.x) >> scale;
			uint shy = floatBitsToUint(pos.y) >> scale;
			uint shz = floatBitsToUint(pos.z) >> scale;
			pos.x = uintBitsToFloat(shx << scale);
			pos.y = uintBitsToFloat(shy << scale);
			pos.z = uintBitsToFloat(shz << scale);
			idx = (shx & 1u) | ((shy & 1u) << 1u) | ((shz & 1u) << 2u);

			// Prevent same parent from being stored again and invalidate cached
			// child descriptor.
			h = 0.0f;
			cur = 0;
		}
	}
	return scale < STACK_SIZE && t_min <= t_max;
}

#endif