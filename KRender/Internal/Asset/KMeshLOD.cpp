#include "KMesh.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KVertexDefinition.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KStringUtil.h"
#include "KBase/Publish/Mesh/KMeshSimplification.h"
#include <cmath>
#include <cstring>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <unistd.h>
#endif

struct KMesh::LODCache
{
	static constexpr uint32_t ReducedLevelCount = KSubMesh::LOD_COUNT - 1;
	using PositionVertex = KVertexDefinition::POS_3F_NORM_3F_UV_2F;
	using ColorVertex = KVertexDefinition::COLOR_3F;
	using TangentVertex = KVertexDefinition::TAN_3F_BIN_3F;

	static constexpr std::array<size_t, AssetVertexFormats.size()> Strides = {
		sizeof(PositionVertex), sizeof(ColorVertex), sizeof(TangentVertex)
	};

	struct Range
	{
		uint32_t vertexStart = 0;
		uint32_t vertexCount = 0;
		uint32_t indexStart = 0;
		uint32_t indexCount = 0;
		float error = 0.0f;
	};

	struct Level
	{
		std::array<KMeshRawData::VertexDataBuffer, AssetVertexFormats.size()> vertices;
		std::vector<uint32_t> indices;
		std::vector<Range> parts;
		uint32_t vertexCount = 0;
	};

	std::array<Level, ReducedLevelCount> levels;

	static_assert(sizeof(PositionVertex) == 32 && sizeof(ColorVertex) == 12 && sizeof(TangentVertex) == 24, "LOD vertex layout");
	static_assert(sizeof(Range) == 20, "LOD range layout");

	static constexpr uint32_t FileMagic = 0x444F4C4B;
	static constexpr uint32_t FileVersion = 1;

	struct Header
	{
		uint32_t magic = FileMagic;
		uint32_t version = FileVersion;
		uint64_t sourceHash = 0;
		uint64_t payloadHash = 0;
		uint32_t partCount = 0;
		uint32_t vertexCounts[ReducedLevelCount] = {};
		uint32_t indexCounts[ReducedLevelCount] = {};
		uint32_t reserved = 0;
	};
	static_assert(sizeof(Header) == 48, "LOD header layout");

	struct Fingerprint
	{
		uint64_t value = 14695981039346656037ull;

		void Add(const void* data, size_t size)
		{
			const auto* bytes = static_cast<const unsigned char*>(data);
			for (size_t offset = 0; offset < size; ++offset)
			{
				value = (value ^ bytes[offset]) * 1099511628211ull;
			}
		}

		template <typename Value>
		void Add(const Value& scalar)
		{
			Add(&scalar, sizeof(scalar));
		}

		template <typename Value>
		void Add(const std::vector<Value>& values)
		{
			Add(values.data(), values.size() * sizeof(Value));
		}
	};

	static uint64_t SourceFingerprint(const KMeshRawData& source)
	{
		Fingerprint hash;
		hash.Add(source.vertexCount);
		hash.Add(source.indexCount);
		hash.Add(source.index16Bit);
		for (const auto& stream : source.verticesDatas)
		{
			hash.Add(stream);
		}
		hash.Add(source.indicesData);
		for (const auto& part : source.parts)
		{
			hash.Add(part.vertexBase);
			hash.Add(part.vertexCount);
			hash.Add(part.indexBase);
			hash.Add(part.indexCount);
			hash.Add(static_cast<uint32_t>(part.material.alphaMode));
		}
		return hash.value;
	}

	uint64_t PayloadFingerprint() const
	{
		Fingerprint hash;
		for (const auto& level : levels)
		{
			hash.Add(level.parts);
			for (const auto& stream : level.vertices)
			{
				hash.Add(stream);
			}
			hash.Add(level.indices);
		}
		return hash.value;
	}

	static bool IsEligible(const KMeshRawData::ModelPart& part)
	{
		return part.material.alphaMode == MAM_OPAQUE && part.indexCount >= 3 && part.indexCount % 3 == 0;
	}

	static uint64_t BufferBytes(uint32_t vertexCount, uint32_t indexCount)
	{
		return uint64_t(vertexCount) * (Strides[0] + Strides[1] + Strides[2]) + uint64_t(indexCount) * sizeof(uint32_t);
	}

	template <typename Vertex>
	static Vertex ReadVertex(const KMeshRawData::VertexDataBuffer& stream, uint32_t index)
	{
		Vertex vertex;
		memcpy(&vertex, stream.data() + size_t(index) * sizeof(Vertex), sizeof(Vertex));
		return vertex;
	}

	template <typename Vertex>
	static void WriteVertex(KMeshRawData::VertexDataBuffer& stream, uint32_t index, const Vertex& vertex)
	{
		memcpy(stream.data() + size_t(index) * sizeof(Vertex), &vertex, sizeof(Vertex));
	}

	static bool IsFinite(const glm::vec3& value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	static bool IsFinite(const KMeshProcessorVertex& vertex)
	{
		return IsFinite(vertex.pos) && IsFinite(vertex.normal) && IsFinite(vertex.color[0])
			&& std::isfinite(vertex.uv.x) && std::isfinite(vertex.uv.y);
	}

	static bool ExtractPart(const KMeshRawData& source, const KMeshRawData::ModelPart& part,
		std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices)
	{
		if (uint64_t(part.indexBase) + part.indexCount > source.indexCount || part.indexCount > INT32_MAX)
		{
			return false;
		}

		const size_t indexSize = source.index16Bit ? sizeof(uint16_t) : sizeof(uint32_t);
		std::unordered_map<uint32_t, uint32_t> remap;
		indices.reserve(part.indexCount);
		for (uint32_t offset = 0; offset < part.indexCount; ++offset)
		{
			const char* data = source.indicesData.data() + (size_t(part.indexBase) + offset) * indexSize;
			uint32_t sourceIndex = 0;
			memcpy(&sourceIndex, data, indexSize);
			if (sourceIndex >= source.vertexCount)
			{
				return false;
			}

			auto entry = remap.emplace(sourceIndex, static_cast<uint32_t>(vertices.size()));
			if (entry.second)
			{
				const auto position = ReadVertex<PositionVertex>(source.verticesDatas[0], sourceIndex);
				const auto color = ReadVertex<ColorVertex>(source.verticesDatas[1], sourceIndex);
				const auto tangent = ReadVertex<TangentVertex>(source.verticesDatas[2], sourceIndex);
				KMeshProcessorVertex vertex{};
				vertex.pos = position.POSITION;
				vertex.normal = position.NORMAL;
				vertex.uv = position.UV;
				vertex.color[0] = color.COLOR;
				vertex.tangent = IsFinite(tangent.TANGENT) ? tangent.TANGENT : glm::vec3(0.0f);
				vertex.binormal = IsFinite(tangent.BINORMAL) ? tangent.BINORMAL : glm::vec3(0.0f);
				if (!IsFinite(vertex))
				{
					return false;
				}
				vertices.push_back(vertex);
			}
			indices.push_back(entry.first->second);
		}

		for (size_t offset = 0; offset < indices.size(); offset += 3)
		{
			const glm::dvec3 origin(vertices[indices[offset]].pos);
			const glm::dvec3 edge1 = glm::dvec3(vertices[indices[offset + 1]].pos) - origin;
			const glm::dvec3 edge2 = glm::dvec3(vertices[indices[offset + 2]].pos) - origin;
			const glm::dvec3 cross = glm::cross(edge1, edge2);
			if (glm::dot(cross, cross) > 0.0)
			{
				return true;
			}
		}
		return false;
	}

	static glm::dvec3 NormalizeOr(const glm::dvec3& value, const glm::dvec3& fallback)
	{
		const double lengthSquared = glm::dot(value, value);
		return lengthSquared > 0.0 && std::isfinite(lengthSquared) ? value / std::sqrt(lengthSquared) : fallback;
	}

	static bool RebuildTangents(std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices)
	{
		struct AccumulatedBasis
		{
			glm::dvec3 normal = glm::dvec3(0.0);
			glm::dvec3 tangent = glm::dvec3(0.0);
			glm::dvec3 binormal = glm::dvec3(0.0);
		};
		std::vector<AccumulatedBasis> bases(vertices.size());
		for (const auto& vertex : vertices)
		{
			if (!IsFinite(vertex))
			{
				return false;
			}
		}

		bool hasArea = false;
		for (size_t offset = 0; offset < indices.size(); offset += 3)
		{
			const uint32_t first = indices[offset], second = indices[offset + 1], third = indices[offset + 2];
			if (std::max(first, std::max(second, third)) >= vertices.size())
			{
				return false;
			}
			const auto& origin = vertices[first];
			const glm::dvec3 edge1 = glm::dvec3(vertices[second].pos) - glm::dvec3(origin.pos);
			const glm::dvec3 edge2 = glm::dvec3(vertices[third].pos) - glm::dvec3(origin.pos);
			const glm::dvec3 faceNormal = glm::cross(edge1, edge2);
			const double area = glm::length(faceNormal);
			hasArea |= area > 0.0;

			const glm::dvec2 uv1 = glm::dvec2(vertices[second].uv) - glm::dvec2(origin.uv);
			const glm::dvec2 uv2 = glm::dvec2(vertices[third].uv) - glm::dvec2(origin.uv);
			const double determinant = uv1.x * uv2.y - uv1.y * uv2.x;
			glm::dvec3 tangent(0.0), binormal(0.0);
			if (determinant != 0.0 && area > 0.0)
			{
				const double sign = determinant > 0.0 ? 1.0 : -1.0;
				tangent = area * NormalizeOr(sign * (uv2.y * edge1 - uv1.y * edge2), glm::dvec3(0.0));
				binormal = area * NormalizeOr(sign * (uv1.x * edge2 - uv2.x * edge1), glm::dvec3(0.0));
			}
			for (uint32_t index : { first, second, third })
			{
				bases[index].normal += faceNormal;
				bases[index].tangent += tangent;
				bases[index].binormal += binormal;
			}
		}

		for (size_t index = 0; index < vertices.size(); ++index)
		{
			auto& vertex = vertices[index];
			const auto& basis = bases[index];
			const glm::dvec3 normal = NormalizeOr(glm::dvec3(vertex.normal), NormalizeOr(basis.normal, glm::dvec3(0.0, 0.0, 1.0)));
			const glm::dvec3 axis = std::abs(normal.x) < 0.9 ? glm::dvec3(1.0, 0.0, 0.0) : glm::dvec3(0.0, 1.0, 0.0);
			const glm::dvec3 tangent = NormalizeOr(basis.tangent - normal * glm::dot(normal, basis.tangent),
				glm::normalize(glm::cross(axis, normal)));
			const glm::dvec3 binormal = glm::cross(normal, tangent);
			vertex.normal = glm::vec3(normal);
			vertex.tangent = glm::vec3(tangent);
			vertex.binormal = glm::vec3(glm::dot(binormal, basis.binormal) < 0.0 ? -binormal : binormal);
		}
		return hasArea;
	}

	static bool AppendPart(Level& level, size_t slot, const std::vector<KMeshProcessorVertex>& vertices,
		const std::vector<uint32_t>& indices, float error)
	{
		if (vertices.size() > UINT32_MAX - level.vertexCount || indices.size() > UINT32_MAX - level.indices.size())
		{
			return false;
		}
		Range& range = level.parts[slot];
		range = { level.vertexCount, static_cast<uint32_t>(vertices.size()),
			static_cast<uint32_t>(level.indices.size()), static_cast<uint32_t>(indices.size()), error };
		level.vertexCount += range.vertexCount;
		for (size_t stream = 0; stream < Strides.size(); ++stream)
		{
			level.vertices[stream].resize(size_t(level.vertexCount) * Strides[stream]);
		}
		for (uint32_t index = 0; index < range.vertexCount; ++index)
		{
			const auto& vertex = vertices[index];
			const uint32_t destination = range.vertexStart + index;
			WriteVertex(level.vertices[0], destination, PositionVertex{ vertex.pos, vertex.normal, vertex.uv });
			WriteVertex(level.vertices[1], destination, ColorVertex{ vertex.color[0] });
			WriteVertex(level.vertices[2], destination, TangentVertex{ vertex.tangent, vertex.binormal });
		}
		for (uint32_t index : indices)
		{
			level.indices.push_back(range.vertexStart + index);
		}
		return true;
	}

	bool Build(const KMeshRawData& source)
	{
		if (source.parts.size() > UINT32_MAX || source.verticesDatas.size() != Strides.size()
			|| source.indicesData.size() != uint64_t(source.indexCount) * (source.index16Bit ? 2 : 4))
		{
			return false;
		}
		for (size_t stream = 0; stream < Strides.size(); ++stream)
		{
			if (source.verticesDatas[stream].size() != uint64_t(source.vertexCount) * Strides[stream])
			{
				return false;
			}
		}
		for (auto& level : levels)
		{
			level.parts.resize(source.parts.size());
		}
		for (size_t slot = 0; slot < source.parts.size(); ++slot)
		{
			const auto& part = source.parts[slot];
			const uint32_t sourceTriangles = part.indexCount / 3;
			if (!IsEligible(part) || sourceTriangles <= 3)
			{
				continue;
			}
			std::vector<KMeshProcessorVertex> vertices;
			std::vector<uint32_t> indices;
			if (!ExtractPart(source, part, vertices, indices))
			{
				continue;
			}

			KMeshSimplification simplifier;
			const int32_t lod2Target = std::max(3u, sourceTriangles / 4);
			if (!simplifier.Init(vertices, indices, std::vector<uint32_t>(sourceTriangles, 0), 3, lod2Target))
			{
				continue;
			}

			size_t previousCount = part.indexCount;
			std::vector<uint32_t> materials;
			const int32_t targets[ReducedLevelCount] = { static_cast<int32_t>(sourceTriangles / 2), lod2Target };
			for (uint32_t level = 0; level < ReducedLevelCount; ++level)
			{
				float error = 0.0f;
				const int32_t target = std::max(targets[level], simplifier.GetMinTriangleCount());
				if (target == static_cast<int32_t>(sourceTriangles) || uint64_t(target) * 3 >= previousCount)
				{
					continue;
				}
				if (!simplifier.Simplify(MeshSimplifyTarget::TRIANGLE, target, vertices, indices, materials, error)
					|| vertices.empty() || indices.empty() || indices.size() % 3 != 0 || indices.size() >= previousCount
					|| !std::isfinite(error) || error < 0.0f || !RebuildTangents(vertices, indices))
				{
					continue;
				}
				if (!AppendPart(levels[level], slot, vertices, indices, error))
				{
					return false;
				}
				previousCount = indices.size();
			}
		}
		return true;
	}

	static bool ReadHeader(IKDataStream& stream, const KMeshRawData& source, Header& header)
	{
		if (stream.GetSize() < sizeof(header) || !stream.Read(&header, sizeof(header))
			|| header.magic != FileMagic || header.version != FileVersion || header.reserved
			|| header.partCount != source.parts.size())
		{
			return false;
		}

		uint64_t eligibleIndices = 0;
		for (const auto& part : source.parts)
		{
			if (IsEligible(part))
			{
				eligibleIndices += part.indexCount;
			}
		}
		uint64_t bytes = sizeof(header) + uint64_t(header.partCount) * sizeof(Range) * ReducedLevelCount;
		for (uint32_t level = 0; level < ReducedLevelCount; ++level)
		{
			if (header.vertexCounts[level] > header.indexCounts[level] || header.indexCounts[level] > eligibleIndices
				|| header.indexCounts[level] % 3 != 0)
			{
				return false;
			}
			bytes += BufferBytes(header.vertexCounts[level], header.indexCounts[level]);
		}
		return bytes == stream.GetSize() && header.sourceHash == SourceFingerprint(source);
	}

	bool ValidateRanges(const KMeshRawData& source) const
	{
		uint32_t vertexStart[ReducedLevelCount] = {};
		uint32_t indexStart[ReducedLevelCount] = {};
		for (size_t slot = 0; slot < source.parts.size(); ++slot)
		{
			const auto& part = source.parts[slot];
			uint32_t previousCount = part.indexCount;
			for (uint32_t level = 0; level < ReducedLevelCount; ++level)
			{
				const auto& data = levels[level];
				const auto& range = data.parts[slot];
				if (!range.indexCount)
				{
					if (range.vertexCount || range.vertexStart || range.indexStart || range.error != 0.0f)
					{
						return false;
					}
					continue;
				}
				if (!IsEligible(part) || range.indexCount % 3 != 0 || range.indexCount >= previousCount
					|| !std::isfinite(range.error) || range.error < 0.0f
					|| range.vertexStart != vertexStart[level] || range.indexStart != indexStart[level]
					|| range.vertexCount < 3 || range.vertexCount > range.indexCount
					|| range.vertexCount > data.vertexCount - vertexStart[level]
					|| range.indexCount > data.indices.size() - indexStart[level])
				{
					return false;
				}
				vertexStart[level] += range.vertexCount;
				indexStart[level] += range.indexCount;
				for (uint32_t offset = range.indexStart; offset < indexStart[level]; ++offset)
				{
					const uint32_t index = data.indices[offset];
					if (index < range.vertexStart || index >= vertexStart[level])
					{
						return false;
					}
				}
				previousCount = range.indexCount;
			}
		}
		for (uint32_t level = 0; level < ReducedLevelCount; ++level)
		{
			if (vertexStart[level] != levels[level].vertexCount || indexStart[level] != levels[level].indices.size())
			{
				return false;
			}
		}
		return true;
	}

	template <typename Value>
	static bool ReadData(IKDataStream& stream, std::vector<Value>& values, uint64_t count)
	{
		if (count > values.max_size())
		{
			return false;
		}
		values.resize(static_cast<size_t>(count));
		return values.empty() || stream.Read(values.data(), values.size() * sizeof(Value));
	}

	bool Save(const std::string& path, const KMeshRawData& source) const
	{
		Header header;
		header.partCount = static_cast<uint32_t>(source.parts.size());
		header.sourceHash = SourceFingerprint(source);
		header.payloadHash = PayloadFingerprint();
		for (uint32_t level = 0; level < ReducedLevelCount; ++level)
		{
			header.vertexCounts[level] = levels[level].vertexCount;
			header.indexCounts[level] = static_cast<uint32_t>(levels[level].indices.size());
		}

		std::string temporary;
#if defined(_WIN32)
		char filename[MAX_PATH];
		const std::string directory = path.substr(0, path.find_last_of("/\\") + 1);
		if (!GetTempFileNameA(directory.c_str(), "lod", 0, filename))
		{
			return false;
		}
		temporary = filename;
#else
		std::vector<char> filename(path.begin(), path.end());
		const char suffix[] = ".tmp.XXXXXX";
		filename.insert(filename.end(), suffix, suffix + sizeof(suffix));
		const int descriptor = mkstemp(filename.data());
		if (descriptor == -1)
		{
			return false;
		}
		close(descriptor);
		temporary = filename.data();
#endif
		FILE* stream = nullptr;
#if defined(_WIN32)
		fopen_s(&stream, temporary.c_str(), "wb");
#else
		stream = fopen(temporary.c_str(), "wb");
#endif
		if (!stream)
		{
			std::remove(temporary.c_str());
			return false;
		}
		auto write = [stream](const void* data, size_t size) { return !size || fwrite(data, 1, size, stream) == size; };
		bool success = write(&header, sizeof(header));
		for (const auto& level : levels)
		{
			success = success && write(level.parts.data(), level.parts.size() * sizeof(Range));
			for (const auto& vertices : level.vertices)
			{
				success = success && write(vertices.data(), vertices.size());
			}
			success = success && write(level.indices.data(), level.indices.size() * sizeof(uint32_t));
		}
		success = (fclose(stream) == 0) && success;
		if (success)
		{
#if defined(_WIN32)
			success = MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
			success = std::rename(temporary.c_str(), path.c_str()) == 0;
#endif
		}
		if (!success)
		{
			std::remove(temporary.c_str());
		}
		return success;
	}

	bool Load(const std::string& path, const KMeshRawData& source)
	{
		const std::string cachePath = path + ".lod";
		IKDataStreamPtr stream;
		for (FileSystemDomain domain : { FSD_RESOURCE, FSD_BACKUP })
		{
			IKFileSystemPtr filesystem = KFileSystem::Manager->GetFileSystem(domain);
			if (!filesystem)
			{
				continue;
			}
			KFileSystemPtrList systems;
			if (filesystem->GetType() == FST_MULTI)
			{
				filesystem->GetAllSubFileSystem(systems);
			}
			else
			{
				systems.push_back(filesystem);
			}
			for (const auto& system : systems)
			{
				if (system->Open(cachePath, IT_FILEHANDLE, stream))
				{
					break;
				}
			}
			if (stream)
			{
				break;
			}
		}
		if (!stream)
		{
			return false;
		}

		Header header;
		if (!ReadHeader(*stream, source, header))
		{
			return false;
		}
		for (uint32_t level = 0; level < ReducedLevelCount; ++level)
		{
			auto& data = levels[level];
			data.vertexCount = header.vertexCounts[level];
			if (!ReadData(*stream, data.parts, header.partCount))
			{
				return false;
			}
			for (size_t binding = 0; binding < Strides.size(); ++binding)
			{
				if (!ReadData(*stream, data.vertices[binding], uint64_t(data.vertexCount) * Strides[binding]))
				{
					return false;
				}
			}
			if (!ReadData(*stream, data.indices, header.indexCounts[level]))
			{
				return false;
			}
		}
		return header.payloadHash == PayloadFingerprint() && ValidateRanges(source);
	}

	void PrintStatistics(const std::string& path, const KMeshRawData& source) const
	{
		uint64_t triangles[KSubMesh::LOD_COUNT] = {};
		uint32_t reducedParts[ReducedLevelCount] = {};
		for (size_t slot = 0; slot < source.parts.size(); ++slot)
		{
			const auto& part = source.parts[slot];
			uint32_t count = (part.indexCount ? part.indexCount : part.vertexCount) / 3;
			triangles[0] += count;
			for (uint32_t level = 0; level < ReducedLevelCount; ++level)
			{
				const auto& range = levels[level].parts[slot];
				if (range.indexCount)
				{
					count = range.indexCount / 3;
					++reducedParts[level];
				}
				triangles[level + 1] += count;
			}
		}
		printf("%s\nLOD0 %llu triangles\n", path.c_str(), static_cast<unsigned long long>(triangles[0]));
		for (uint32_t level = 0; level < ReducedLevelCount; ++level)
		{
			printf("LOD%u %llu triangles, %u reduced parts, %llu extra buffer bytes\n", level + 1,
				static_cast<unsigned long long>(triangles[level + 1]), reducedParts[level],
				static_cast<unsigned long long>(BufferBytes(levels[level].vertexCount, static_cast<uint32_t>(levels[level].indices.size()))));
		}
	}
};

bool KMesh::BakeLOD(const std::string& path, std::string& error)
{
	std::string stem, extension;
	KFileTool::SplitExt(path, stem, extension);
	KStringUtil::Lower(extension, extension);
	if (extension != ".gltf" && extension != ".glb" && extension != ".obj")
	{
		error = "LOD baking requires a local glTF, GLB or OBJ asset";
		return false;
	}

	std::string assetPath;
#if defined(_WIN32)
	const DWORD length = GetFullPathNameA(path.c_str(), 0, nullptr, nullptr);
	assetPath.resize(length);
	const DWORD written = length ? GetFullPathNameA(path.c_str(), length, &assetPath[0], nullptr) : 0;
	if (!written || written >= length)
	{
		error = "Cannot resolve asset path: " + path;
		return false;
	}
	assetPath.resize(written);
#else
	if (path.front() == '/')
	{
		assetPath = path;
	}
	else if (!KFileTool::AbsPath(path, assetPath))
	{
		error = "Cannot resolve asset path: " + path;
		return false;
	}
#endif
	const size_t filename = assetPath.find_last_of("/\\") + 1;
	KFileSystem::CreateFileManager();
	KLog::CreateLogger();
	KLog::Logger->Init(nullptr, true, false, ILM_UNIX);
	IKFileSystemPtr filesystem = KFileSystem::CreateFileSystem(FST_NATIVE);
	filesystem->SetRoot(assetPath.substr(0, filename));
	KFileSystem::Manager->SetFileSystem(FSD_RESOURCE, filesystem);
	KMeshRawData source;
	const bool imported = ImportAsset("./" + assetPath.substr(filename), source);
	KFileSystem::DestroyFileManager();
	KLog::Logger->UnInit();
	KLog::DestroyLogger();
	if (!imported)
	{
		error = "Cannot import asset: " + path;
		return false;
	}

	LODCache cache;
	if (!cache.Build(source))
	{
		error = "Cannot build LOD buffers for asset: " + path;
		return false;
	}
	const std::string cachePath = assetPath + ".lod";
	if (!cache.Save(cachePath, source))
	{
		error = "Cannot write LOD cache: " + cachePath;
		return false;
	}
	cache.PrintStatistics(cachePath, source);
	return true;
}

void KMesh::LoadLODs(const KMeshRawData& source)
{
	LODCache cache;
	if (!cache.Load(m_Path, source))
	{
		return;
	}

	for (size_t level = 0; level < cache.levels.size(); ++level)
	{
		const auto& data = cache.levels[level];
		if (data.indices.empty())
		{
			continue;
		}
		auto& buffers = m_LODBuffers[level];
		const std::string label = m_Path + "_LOD" + std::to_string(level + 1);
		buffers.vertices.vertexCount = data.vertexCount;
		buffers.vertices.bound = m_VertexData.bound;
		buffers.vertices.vertexFormats = m_VertexData.vertexFormats;
		buffers.vertices.vertexBuffers.resize(data.vertices.size());
		for (size_t stream = 0; stream < data.vertices.size(); ++stream)
		{
			auto& buffer = buffers.vertices.vertexBuffers[stream];
			const auto& detail = KVertexDefinition::GetVertexDetail(buffers.vertices.vertexFormats[stream]);
			if (!KRenderGlobal::RenderDevice->CreateVertexBuffer(buffer) ||
				!buffer->InitMemory(data.vertexCount, detail.vertexSize, data.vertices[stream].data()) ||
				!buffer->InitDevice(false))
			{
				UnInitLODs();
				return;
			}
			buffer->SetDebugName((label + "_VB_" + std::to_string(stream)).c_str());
			if (KRenderGlobal::InEditor)
			{
				buffer->DiscardMemory();
			}
		}
		if (!KRenderGlobal::RenderDevice->CreateIndexBuffer(buffers.indices) ||
			!buffers.indices->InitMemory(IT_32, static_cast<uint32_t>(data.indices.size()), data.indices.data()) ||
			!buffers.indices->InitDevice(false))
		{
			UnInitLODs();
			return;
		}
		buffers.indices->SetDebugName((label + "_IB").c_str());
		if (KRenderGlobal::InEditor)
		{
			buffers.indices->DiscardMemory();
		}

		for (size_t slot = 0; slot < data.parts.size(); ++slot)
		{
			const auto& range = data.parts[slot];
			if (!range.indexCount)
			{
				continue;
			}
			auto& base = m_SubMeshes[slot];
			auto lod = std::make_shared<KSubMesh>(this);
			lod->m_Material = m_SubMaterials[slot];
			lod->m_pVertexData = &buffers.vertices;
			lod->m_IndexData.indexBuffer = buffers.indices;
			lod->m_IndexData.indexStart = range.indexStart;
			lod->m_IndexData.indexCount = range.indexCount;
			lod->m_OwnIndexBuffer = false;
			lod->m_NeedAccelerationStructure = false;
			lod->m_Bound = base->m_Bound;
			base->m_ReducedLODs[level] = std::move(lod);
		}
	}
}
