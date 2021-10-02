/*
 * Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2017-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>
#include <KBase/Publish/KNumerical.h>

#define KMESHLETS_MAX_VERTEX_COUNT_LIMIT 256
#define KMESHLETS_MAX_PRIMITIVE_COUNT_LIMIT 256
#define KMESHLETS_PER_TASK 32
#define KMESHLETS_PACKBASIC_ALIGN 16
 // how many indices are fetched per thread, 8 or 4
#define KMESHLETS_PACKBASIC_PRIMITIVE_INDICES_PER_FETCH 8

// must not change
typedef uint8_t  KMeshletPrimitiveIndexType; // must store [0, MAX_VERTEX_COUNT_LIMIT-1]
typedef uint32_t KMeshletPackBasicType;

struct KMeshletBbox
{
	float bboxMin[3];
	float bboxMax[3];

	KMeshletBbox()
	{
		bboxMin[0] = FLT_MAX;
		bboxMin[1] = FLT_MAX;
		bboxMin[2] = FLT_MAX;
		bboxMax[0] = -FLT_MAX;
		bboxMax[1] = -FLT_MAX;
		bboxMax[2] = -FLT_MAX;
	}
};

struct KMeshletPrimitiveCache
{
	//  Utility class to generate the meshlets from triangle indices.
	//  It finds the unique vertex set used by a series of primitives.
	//  The cache is exhausted if either of the maximums is hit.
	//  The effective limits used with the cache must be < MAX.

	KMeshletPrimitiveIndexType  primitives[KMESHLETS_MAX_PRIMITIVE_COUNT_LIMIT][3];
	uint32_t vertices[KMESHLETS_MAX_VERTEX_COUNT_LIMIT];
	uint32_t numPrims;
	uint32_t numVertices;
	uint32_t numVertexDeltaBits;
	uint32_t numVertexAllBits;

	uint32_t maxVertexSize;
	uint32_t maxPrimitiveSize;
	uint32_t primitiveBits = 1;
	uint32_t maxBlockBits = ~0;

	bool Empty() const { return numVertices == 0; }

	void Reset()
	{
		numPrims = 0;
		numVertices = 0;
		numVertexDeltaBits = 0;
		numVertexAllBits = 0;
		// reset
		memset(vertices, 0xFFFFFFFF, sizeof(vertices));
	}

	bool FitsBlock() const
	{
		uint32_t primBits = (numPrims - 1) * 3 * primitiveBits;
		uint32_t vertBits = (numVertices - 1) * numVertexDeltaBits;
		bool     state = (primBits + vertBits) <= maxBlockBits;
		return state;
	}

	bool CannotInsert(uint32_t idxA, uint32_t idxB, uint32_t idxC) const
	{
		const uint32_t indices[3] = { idxA, idxB, idxC };
		// skip degenerate
		if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
		{
			return false;
		}

		uint32_t found = 0;
		for (uint32_t v = 0; v < numVertices; v++)
		{
			for (int i = 0; i < 3; i++)
			{
				uint32_t idx = indices[i];
				if (vertices[v] == idx)
				{
					found++;
				}
			}
		}
		// out of bounds
		return (numVertices + 3 - found) > maxVertexSize || (numPrims + 1) > maxPrimitiveSize;
	}

	bool CannotInsertBlock(uint32_t idxA, uint32_t idxB, uint32_t idxC) const
	{
		const uint32_t indices[3] = { idxA, idxB, idxC };
		// skip degenerate
		if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
		{
			return false;
		}

		uint32_t found = 0;
		for (uint32_t v = 0; v < numVertices; v++)
		{
			for (int i = 0; i < 3; i++)
			{
				uint32_t idx = indices[i];
				if (vertices[v] == idx)
				{
					found++;
				}
			}
		}
		// ensure one bit is set in deltas for findMSB returning 0
		uint32_t firstVertex = numVertices ? vertices[0] : indices[0];
		uint32_t cmpBits = std::max(KNumerical::FindMSB((firstVertex ^ indices[0]) | 1),
			std::max(KNumerical::FindMSB((firstVertex ^ indices[1]) | 1), KNumerical::FindMSB((firstVertex ^ indices[2]) | 1)))
			+ 1;

		uint32_t deltaBits = std::max(cmpBits, numVertexDeltaBits);

		uint32_t newVertices = numVertices + 3 - found;
		uint32_t newPrims = numPrims + 1;

		uint32_t newVertBits = (newVertices - 1) * deltaBits;
		uint32_t newPrimBits = (newPrims - 1) * 3 * primitiveBits;
		uint32_t newBits = newVertBits + newPrimBits;

		// out of bounds
		return (newPrims > maxPrimitiveSize) || (newVertices > maxVertexSize) || (newBits > maxBlockBits);
	}

	void Insert(uint32_t idxA, uint32_t idxB, uint32_t idxC)
	{
		const uint32_t indices[3] = { idxA, idxB, idxC };
		uint32_t       tri[3];

		// skip degenerate
		if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
		{
			return;
		}

		for (int i = 0; i < 3; i++)
		{
			uint32_t idx = indices[i];
			bool     found = false;
			for (uint32_t v = 0; v < numVertices; v++)
			{
				if (idx == vertices[v])
				{
					tri[i] = v;
					found = true;
					break;
				}
			}
			if (!found)
			{
				vertices[numVertices] = idx;
				tri[i] = numVertices;

				if (numVertices)
				{
					numVertexDeltaBits = std::max((uint32_t)(KNumerical::FindMSB((idx ^ vertices[0]) | 1) + 1), numVertexDeltaBits);
				}
				numVertexAllBits = std::max(numVertexAllBits, (uint32_t)(KNumerical::FindMSB(idx) + 1));

				numVertices++;
			}
		}

		primitives[numPrims][0] = tri[0];
		primitives[numPrims][1] = tri[1];
		primitives[numPrims][2] = tri[2];
		numPrims++;

		assert(FitsBlock());
	}
};

struct KMeshletPackBasicDesc
{
	//
	// Bitfield layout :
	//
	//   Field.X    | Bits | Content
	//  ------------|:----:|----------------------------------------------
	//  bboxMinX    | 8    | bounding box coord relative to object bbox
	//  bboxMinY    | 8    | UNORM8
	//  bboxMinZ    | 8    |
	//  vertexMax   | 8    | number of vertex indices - 1 in the meshlet
	//  ------------|:----:|----------------------------------------------
	//   Field.Y    |      |
	//  ------------|:----:|----------------------------------------------
	//  bboxMaxX    | 8    | bounding box coord relative to object bbox
	//  bboxMaxY    | 8    | UNORM8
	//  bboxMaxZ    | 8    |
	//  primMax     | 8    | number of primitives - 1 in the meshlet
	//  ------------|:----:|----------------------------------------------
	//   Field.Z    |      |
	//  ------------|:----:|----------------------------------------------
	//  coneOctX    | 8    | octant coordinate for cone normal, SNORM8
	//  coneOctY    | 8    | octant coordinate for cone normal, SNORM8
	//  coneAngle   | 8    | -sin(cone.angle),  SNORM8
	//  vertexPack  | 8    | vertex indices per 32 bits (1 or 2)
	//  ------------|:----:|----------------------------------------------
	//   Field.W    |      |
	//  ------------|:----:|----------------------------------------------
	//  packOffset  | 32   | index buffer value of the first vertex

	//
	// Note : the bitfield is not expanded in the struct due to differences in how
	//        GPU & CPU compilers pack bit-fields and endian-ness.

	union
	{
#if !defined(NDEBUG) && defined(_MSC_VER)
		struct
		{
			// warning, not portable
			unsigned bboxMinX : 8;
			unsigned bboxMinY : 8;
			unsigned bboxMinZ : 8;
			unsigned vertexMax : 8;

			unsigned bboxMaxX : 8;
			unsigned bboxMaxY : 8;
			unsigned bboxMaxZ : 8;
			unsigned primMax : 8;

			signed   coneOctX : 8;
			signed   coneOctY : 8;
			signed   coneAngle : 8;
			unsigned vertexPack : 8;

			unsigned packOffset : 32;
		} _debug;
#endif
		struct
		{
			uint32_t fieldX;
			uint32_t fieldY;
			uint32_t fieldZ;
			uint32_t fieldW;
		};
	};

	static uint32_t Pack(uint32_t value, int width, int offset)
	{
		return (uint32_t)((value & ((1 << width) - 1)) << offset);
	}

	static uint32_t Unpack(uint32_t value, int width, int offset)
	{
		return (uint32_t)((value >> offset) & ((1 << width) - 1));
	}

	uint32_t GetNumVertices() const { return Unpack(fieldX, 8, 24) + 1; }
	void     SetNumVertices(uint32_t num)
	{
		assert(num <= KMESHLETS_MAX_VERTEX_COUNT_LIMIT);
		fieldX |= Pack(num - 1, 8, 24);
	}

	uint32_t GetNumPrims() const { return Unpack(fieldY, 8, 24) + 1; }
	void     SetNumPrims(uint32_t num)
	{
		assert(num <= KMESHLETS_MAX_PRIMITIVE_COUNT_LIMIT);
		fieldY |= Pack(num - 1, 8, 24);
	}

	uint32_t GetNumVertexPack() const { return Unpack(fieldZ, 8, 24); }
	void     SetNumVertexPack(uint32_t num) { fieldZ |= Pack(num, 8, 24); }

	uint32_t GetPackOffset() const { return fieldW; }
	void     SetPackOffset(uint32_t index) { fieldW = index; }

	uint32_t GetVertexStart() const { return 0; }
	uint32_t GetVertexSize() const
	{
		uint32_t vertexDiv = GetNumVertexPack();
		uint32_t vertexElems = ((GetNumVertices() + vertexDiv - 1) / vertexDiv);

		return vertexElems;
	}

	uint32_t GetPrimStart() const { return (GetVertexStart() + GetVertexSize() + 1) & (~1u); }
	uint32_t GetPrimSize() const
	{
		uint32_t primDiv = 4;
		uint32_t primElems = ((GetNumPrims() * 3 + KMESHLETS_PACKBASIC_PRIMITIVE_INDICES_PER_FETCH - 1) / primDiv);

		return primElems;
	}

	// positions are relative to object's bbox treated as UNORM
	void SetBBox(uint8_t const bboxMin[3], uint8_t const bboxMax[3])
	{
		fieldX |= Pack(bboxMin[0], 8, 0) | Pack(bboxMin[1], 8, 8) | Pack(bboxMin[2], 8, 16);
		fieldY |= Pack(bboxMax[0], 8, 0) | Pack(bboxMax[1], 8, 8) | Pack(bboxMax[2], 8, 16);
	}

	void GetBBox(uint8_t bboxMin[3], uint8_t bboxMax[3]) const
	{
		bboxMin[0] = Unpack(fieldX, 8, 0);
		bboxMin[0] = Unpack(fieldX, 8, 8);
		bboxMin[0] = Unpack(fieldX, 8, 16);

		bboxMax[0] = Unpack(fieldY, 8, 0);
		bboxMax[0] = Unpack(fieldY, 8, 8);
		bboxMax[0] = Unpack(fieldY, 8, 16);
	}

	// uses octant encoding for cone Normal
	// positive angle means the cluster cannot be backface-culled
	// numbers are treated as SNORM
	void SetCone(int8_t coneOctX, int8_t coneOctY, int8_t minusSinAngle)
	{
		uint8_t anglebits = minusSinAngle;
		fieldZ |= Pack(coneOctX, 8, 0);
		fieldZ |= Pack(coneOctY, 8, 8);
		fieldZ |= Pack(minusSinAngle, 8, 16);
	}

	void getCone(int8_t& coneOctX, int8_t& coneOctY, int8_t& minusSinAngle) const
	{
		coneOctX = Unpack(fieldZ, 8, 0);
		coneOctY = Unpack(fieldZ, 8, 8);
		minusSinAngle = Unpack(fieldZ, 8, 16);
	}

	KMeshletPackBasicDesc()
	{
		fieldX = 0;
		fieldY = 0;
		fieldZ = 0;
		fieldW = 0;
	}
};

struct KMeshletPackBasic
{
	// variable size
	//
	// aligned to PACKBASIC_ALIGN bytes
	// - first squence is either 16 or 32 bit indices per vertex
	//   (vertexPack is 2 or 1) respectively
	// - second sequence aligned to 8 bytes, primitive many 8 bit values
	//   
	//
	// { u32[numVertices/vertexPack ...], padding..., u8[(numPrimitives) * 3 ...] }

	union
	{
		uint32_t data32[1];
		uint16_t data16[1];
		uint8_t  data8[1];
	};

	inline void SetVertexIndex(uint32_t PACKED_SIZE, uint32_t vertex, uint32_t vertexPack, uint32_t indexValue)
	{
#if 1
		if (vertexPack == 1) {
			data32[vertex] = indexValue;
		}
		else {
			data16[vertex] = indexValue;
		}
#else
		uint32_t idx = vertex / vertexPack;
		uint32_t shift = vertex % vertexPack;

		assert(idx < PACKED_SIZE);

		data32[idx] |= indexValue << (shift * 16);
#endif
	}

	inline uint32_t GetVertexIndex(uint32_t vertex, uint32_t vertexPack) const
	{
#if 1
		return (vertexPack == 1) ? data32[vertex] : data16[vertex];
#else
		uint32_t idx = vertex / vertexPack;
		uint32_t shift = vertex & (vertexPack - 1);
		uint32_t bits = vertexPack == 2 ? 16 : 0;

		uint32_t indexValue = data32[idx];
		indexValue <<= ((1 - shift) * bits);
		indexValue >>= (bits);
		return indexValue;
#endif
	}

	inline void SetPrimIndices(uint32_t PACKED_SIZE, uint32_t prim, uint32_t primStart, const uint8_t indices[3])
	{
		uint32_t idx = primStart * 4 + prim * 3;

		assert(idx < PACKED_SIZE * 4);

		data8[idx + 0] = indices[0];
		data8[idx + 1] = indices[1];
		data8[idx + 2] = indices[2];
	}

	inline void GetPrimIndices(uint32_t prim, uint32_t primStart, uint8_t indices[3]) const
	{
		uint32_t idx = primStart * 4 + prim * 3;

		indices[0] = data8[idx + 0];
		indices[1] = data8[idx + 1];
		indices[2] = data8[idx + 2];
	}
};

struct KMeshletGeometry
{
	std::vector<KMeshletPackBasicType> meshletPacks;
	std::vector<KMeshletPackBasicDesc> meshletDescriptors;
	std::vector<KMeshletBbox>          meshletBboxes;

	void Reset()
	{
		meshletPacks.clear();
		meshletDescriptors.clear();
		meshletBboxes.clear();
	}
};

enum KMeshletStatusCode
{
	KMESHLET_STATUS_NO_ERROR,
	KMESHLET_STATUS_PRIM_OUT_OF_BOUNDS,
	KMESHLET_STATUS_VERTEX_OUT_OF_BOUNDS,
	KMESHLET_STATUS_MISMATCH_INDICES,
};

class KMeshletPackBasicBuilder
{
	// Builder configuration
protected:
	// might want to template these instead of using MAX
	uint32_t m_MaxVertexCount;
	uint32_t m_MaxPrimitiveCount;
	bool     m_SeparateBboxes;

	// due to hw allocation granuarlity, good values are
	// vertex count = 32 or 64
	// primitive count = 40, 84 or 126
	// maximizes the fit into gl_PrimitiveIndices[128 * N - 4]
public:
	KMeshletPackBasicBuilder()
		: m_MaxVertexCount(64)
		, m_MaxPrimitiveCount(84)
		, m_SeparateBboxes(false)
	{

	}

	void Setup(uint32_t maxVertexCount, uint32_t maxPrimitiveCount, bool separateBboxes = false)
	{
		assert(maxPrimitiveCount <= KMESHLETS_MAX_PRIMITIVE_COUNT_LIMIT);
		assert(maxVertexCount <= KMESHLETS_MAX_VERTEX_COUNT_LIMIT);

		m_MaxVertexCount = maxVertexCount;
		m_MaxPrimitiveCount = maxPrimitiveCount;
		m_SeparateBboxes = separateBboxes;

		uint32_t indices = maxPrimitiveCount * 3;
		// align to PRIMITIVE_INDICES_PER_FETCH
		uint32_t indicesFit = (indices / KMESHLETS_PACKBASIC_PRIMITIVE_INDICES_PER_FETCH) * KMESHLETS_PACKBASIC_PRIMITIVE_INDICES_PER_FETCH;
		uint32_t numTrisFit = indicesFit / 3;

		assert(numTrisFit > 0);
		m_MaxPrimitiveCount = numTrisFit;
	}
	//////////////////////////////////////////////////////////////////////////
	// generate meshlets
private:
	void AddMeshlet(KMeshletGeometry& geometry, const KMeshletPrimitiveCache& cache) const
	{
		uint32_t packOffset = uint32_t(geometry.meshletPacks.size());
		uint32_t vertexPack = cache.numVertexAllBits <= 16 ? 2 : 1;

		KMeshletPackBasicDesc meshlet;
		meshlet.SetNumPrims(cache.numPrims);
		meshlet.SetNumVertices(cache.numVertices);
		meshlet.SetNumVertexPack(vertexPack);
		meshlet.SetPackOffset(packOffset);

		uint32_t vertexStart = meshlet.GetVertexStart();
		uint32_t vertexSize = meshlet.GetVertexSize();

		uint32_t primStart = meshlet.GetPrimStart();
		uint32_t primSize = meshlet.GetPrimSize();

		assert(primStart + primSize > vertexStart + vertexSize);
		uint32_t packedSize = primStart + primSize;//std::max(vertexStart + vertexSize, primStart + primSize);
		packedSize = KNumerical::AlignedSize<uint32_t>(packedSize, KMESHLETS_PACKBASIC_ALIGN);

		geometry.meshletPacks.resize(geometry.meshletPacks.size() + packedSize, 0);
		geometry.meshletDescriptors.push_back(meshlet);

		KMeshletPackBasic* pack = (KMeshletPackBasic*)&geometry.meshletPacks[packOffset];

		{
			for (uint32_t v = 0; v < cache.numVertices; v++)
			{
				pack->SetVertexIndex(packedSize, v, vertexPack, cache.vertices[v]);
			}

			uint32_t primStart = meshlet.GetPrimStart();

			for (uint32_t p = 0; p < cache.numPrims; p++)
			{
				pack->SetPrimIndices(packedSize, p, primStart, cache.primitives[p]);
			}
		}
	}
public:
	// Returns the number of successfully processed indices.
	// If the returned number is lower than provided input, use the number
	// as starting offset and create a new geometry description.
	template <class VertexIndexType>
	uint32_t BuildMeshlets(KMeshletGeometry& geometry, const uint32_t numIndices, const VertexIndexType* indices) const
	{
		assert(m_MaxPrimitiveCount <= KMESHLETS_MAX_PRIMITIVE_COUNT_LIMIT);
		assert(m_MaxVertexCount <= KMESHLETS_MAX_VERTEX_COUNT_LIMIT);

		KMeshletPrimitiveCache cache;
		cache.maxPrimitiveSize = m_MaxPrimitiveCount;
		cache.maxVertexSize = m_MaxVertexCount;
		cache.Reset();

		for (uint32_t i = 0; i < numIndices / 3; i++)
		{
			if (cache.CannotInsertBlock(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2]))
			{
				// finish old and reset
				AddMeshlet(geometry, cache);
				cache.Reset();
			}
			cache.Insert(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2]);
		}
		if (!cache.Empty())
		{
			AddMeshlet(geometry, cache);
		}

		return numIndices;
	}

	void PadTaskMeshlets(KMeshletGeometry& geometry) const
	{
		if (geometry.meshletDescriptors.empty())
			return;

		// ensure we never have out-of-bounds memory access to array within task shader
		for (uint32_t i = 0; i < KMESHLETS_PER_TASK - 1; i++)
		{
			geometry.meshletDescriptors.push_back(KMeshletPackBasicDesc());
		}
	}

	// bbox and cone angle
	void BuildMeshletEarlyCulling(KMeshletGeometry& geometry,
		const glm::vec3& objectBboxMax,
		const glm::vec3& objectBboxMin,
		const float* positions,
		const size_t positionStride) const
	{
		assert((positionStride % sizeof(float)) == 0);

		size_t positionMul = positionStride / sizeof(float);

		glm::vec3 objectBboxExtent = objectBboxMax - objectBboxMin;

		if (m_SeparateBboxes)
		{
			geometry.meshletBboxes.resize(geometry.meshletDescriptors.size());
		}

		for (size_t i = 0; i < geometry.meshletDescriptors.size(); i++)
		{
			KMeshletPackBasicDesc& meshlet = geometry.meshletDescriptors[i];
			const KMeshletPackBasic* pack = (const KMeshletPackBasic*)&geometry.meshletPacks[meshlet.GetPackOffset()];

			uint32_t primCount = meshlet.GetNumPrims();
			uint32_t primStart = meshlet.GetPrimStart();
			uint32_t vertexCount = meshlet.GetNumVertices();
			uint32_t vertexPack = meshlet.GetNumVertexPack();

			glm::vec3 bboxMin = glm::vec3(FLT_MAX);
			glm::vec3 bboxMax = glm::vec3(-FLT_MAX);

			glm::vec3 avgNormal = glm::vec3(0.0f);
			glm::vec3 triNormals[KMESHLETS_MAX_PRIMITIVE_COUNT_LIMIT];

			// skip unset
			if (vertexCount == 1)
				continue;

			for (uint32_t p = 0; p < primCount; p++)
			{
				uint8_t  indices[3];
				pack->GetPrimIndices(p, primStart, indices);

				uint32_t idxA = pack->GetVertexIndex(indices[0], vertexPack);
				uint32_t idxB = pack->GetVertexIndex(indices[1], vertexPack);
				uint32_t idxC = pack->GetVertexIndex(indices[2], vertexPack);

				glm::vec3 posA = glm::vec3(positions[idxA * positionMul], positions[idxA * positionMul + 1], positions[idxA * positionMul + 2]);
				glm::vec3 posB = glm::vec3(positions[idxB * positionMul], positions[idxB * positionMul + 1], positions[idxB * positionMul + 2]);
				glm::vec3 posC = glm::vec3(positions[idxC * positionMul], positions[idxC * positionMul + 1], positions[idxC * positionMul + 2]);

				{
					// bbox
					bboxMin = glm::min(bboxMin, posA);
					bboxMin = glm::min(bboxMin, posB);
					bboxMin = glm::min(bboxMin, posC);

					bboxMax = glm::max(bboxMax, posA);
					bboxMax = glm::max(bboxMax, posB);
					bboxMax = glm::max(bboxMax, posC);
				}

				{
					// cone
					glm::vec3 cross = glm::cross(posB - posA, posC - posA);
					float length = glm::length(cross);

					glm::vec3 normal;
					if (length > FLT_EPSILON)
					{
						normal = cross * (1.0f / length);
					}
					else
					{
						normal = cross;
					}

					avgNormal = avgNormal + normal;
					triNormals[p] = normal;
				}
			}

			if (m_SeparateBboxes)
			{
				geometry.meshletBboxes[i].bboxMin[0] = bboxMin.x;
				geometry.meshletBboxes[i].bboxMin[1] = bboxMin.y;
				geometry.meshletBboxes[i].bboxMin[2] = bboxMin.z;
				geometry.meshletBboxes[i].bboxMax[0] = bboxMax.x;
				geometry.meshletBboxes[i].bboxMax[1] = bboxMax.y;
				geometry.meshletBboxes[i].bboxMax[2] = bboxMax.z;
			}

			{
				// bbox
				// truncate min relative to object min
				bboxMin = bboxMin - glm::vec3(objectBboxMin);
				bboxMax = bboxMax - glm::vec3(objectBboxMin);
				bboxMin = bboxMin / objectBboxExtent;
				bboxMax = bboxMax / objectBboxExtent;

				// snap to grid
				const int gridBits = 8;
				const int gridLast = (1 << gridBits) - 1;
				uint8_t   gridMin[3];
				uint8_t   gridMax[3];

				gridMin[0] = std::max(0, std::min(int(truncf(bboxMin.x * float(gridLast))), gridLast - 1));
				gridMin[1] = std::max(0, std::min(int(truncf(bboxMin.y * float(gridLast))), gridLast - 1));
				gridMin[2] = std::max(0, std::min(int(truncf(bboxMin.z * float(gridLast))), gridLast - 1));
				gridMax[0] = std::max(0, std::min(int(ceilf(bboxMax.x * float(gridLast))), gridLast));
				gridMax[1] = std::max(0, std::min(int(ceilf(bboxMax.y * float(gridLast))), gridLast));
				gridMax[2] = std::max(0, std::min(int(ceilf(bboxMax.z * float(gridLast))), gridLast));

				meshlet.SetBBox(gridMin, gridMax);
			}

#if 0
			{
				// potential improvement, instead of average maybe use
				// http://www.cs.technion.ac.il/~cggc/files/gallery-pdfs/Barequet-1.pdf

				float len = glm::length(avgNormal);
				if (len > FLT_EPSILON)
				{
					avgNormal = avgNormal / len;
				}
				else
				{
					avgNormal = glm::vec3(0.0f);
				}

				glm::vec3 packed = float32x3_to_octn_precise(avgNormal, 16);
				int8_t coneX = std::min(127, std::max(-127, int32_t(packed.x * 127.0f)));
				int8_t coneY = std::min(127, std::max(-127, int32_t(packed.y * 127.0f)));

				// post quantization normal
				avgNormal = oct_to_float32x3(vec(float(coneX) / 127.0f, float(coneY) / 127.0f, 0.0f));

				float mindot = 1.0f;
				for (unsigned int p = 0; p < primCount; p++)
				{
					mindot = std::min(mindot, glm::dot(triNormals[p], avgNormal));
				}

				// apply safety delta due to quantization
				mindot -= 1.0f / 127.0f;
				mindot = std::max(-1.0f, mindot);

				// positive value for cluster not being backface cullable (normals > 90°)
				int8_t coneAngle = 127;
				if (mindot > 0)
				{
					// otherwise store -sin(cone angle)
					// we test against dot product (cosine) so this is equivalent to cos(cone angle + 90°)
					float angle = -sinf(acosf(mindot));
					coneAngle = std::max(-127, std::min(127, int32_t(angle * 127.0f)));
				}

				meshlet.SetCone(coneX, coneY, coneAngle);
			}
#endif
		}
	}

	template <class VertexIndexType>
	KMeshletStatusCode ErrorCheck(const KMeshletGeometry& geometry,
		uint32_t minVertex,
		uint32_t maxVertex,
		uint32_t numIndices,
		const VertexIndexType* indices) const
	{
		uint32_t compareTris = 0;

		for (size_t i = 0; i < geometry.meshletDescriptors.size(); i++)
		{
			const KMeshletPackBasicDesc& meshlet = geometry.meshletDescriptors[i];
			const KMeshletPackBasic* pack = (const KMeshletPackBasic*)&geometry.meshletPacks[meshlet.GetPackOffset()];

			uint32_t primCount = meshlet.GetNumPrims();
			uint32_t primStart = meshlet.GetPrimStart();
			uint32_t vertexCount = meshlet.GetNumVertices();
			uint32_t vertexPack = meshlet.GetNumVertexPack();

			// skip unset
			if (vertexCount == 1)
				continue;

			for (uint32_t p = 0; p < primCount; p++)
			{
				uint8_t blockIndices[3];
				pack->GetPrimIndices(p, primStart, blockIndices);

				if (blockIndices[0] >= m_MaxVertexCount || blockIndices[1] >= m_MaxVertexCount || blockIndices[2] >= m_MaxVertexCount)
				{
					return KMESHLET_STATUS_PRIM_OUT_OF_BOUNDS;
				}

				uint32_t idxA = pack->GetVertexIndex(blockIndices[0], vertexPack);
				uint32_t idxB = pack->GetVertexIndex(blockIndices[1], vertexPack);
				uint32_t idxC = pack->GetVertexIndex(blockIndices[2], vertexPack);

				if (idxA < minVertex || idxA > maxVertex || idxB < minVertex || idxB > maxVertex || idxC < minVertex || idxC > maxVertex)
				{
					return KMESHLET_STATUS_VERTEX_OUT_OF_BOUNDS;
				}

				uint32_t refA = 0;
				uint32_t refB = 0;
				uint32_t refC = 0;

				while (refA == refB || refA == refC || refB == refC)
				{
					if (compareTris * 3 + 2 >= numIndices)
					{
						return KMESHLET_STATUS_MISMATCH_INDICES;
					}
					refA = indices[compareTris * 3 + 0];
					refB = indices[compareTris * 3 + 1];
					refC = indices[compareTris * 3 + 2];
					compareTris++;
				}

				if (refA != idxA || refB != idxB || refC != idxC)
				{
					return KMESHLET_STATUS_MISMATCH_INDICES;
				}
			}
		}

		return KMESHLET_STATUS_NO_ERROR;
	}
};