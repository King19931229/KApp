#pragma once
#include "KBase/Publish/Mesh/KMeshProcessor.h"
#include <unordered_set>
#include <unordered_map>
#include <functional>

#define ENABLE_POSITION_HASH_KEY_DEBUG 1

struct KPositionHashKey
{
	size_t hash = 0;
#if ENABLE_POSITION_HASH_KEY_DEBUG
	size_t debug = 0;
	glm::vec3 position;
#endif

	KPositionHashKey()
	{}

	KPositionHashKey(size_t h)
	{
		hash = h;
	}

	bool operator<(const KPositionHashKey& other) const
	{
		return hash < other.hash;
	}

	bool operator==(const KPositionHashKey& other) const
	{
		return hash == other.hash;
	}

	bool operator!=(const KPositionHashKey& other) const
	{
		return !(hash == other.hash);
	}
};

template<>
struct std::hash<KPositionHashKey>
{
	inline std::size_t operator()(const KPositionHashKey& key) const
	{
		return key.hash;
	}
};

struct KPositionHash
{
	struct VertexInfomation
	{
		std::unordered_set<size_t> vertices;
		std::unordered_set<size_t> adjacencies;
		uint32_t flag = 0;
		int32_t version = 0;
	};

	std::unordered_map<KPositionHashKey, VertexInfomation> informations;

	KPositionHash()
	{
	}

	void Init()
	{
		informations.clear();
	}

	void UnInit()
	{
		informations.clear();
	}

	KPositionHashKey GetPositionHash(const glm::vec3& position) const
	{
		KPositionHashKey key;
		key.hash = PositionHash(position);
#if ENABLE_POSITION_HASH_KEY_DEBUG
		key.position = position;
		auto it = informations.find(key);
		if (it != informations.end())
		{
			key.debug = it->first.debug;
		}
#endif
		return key;
	}

	void SetFlag(const KPositionHashKey& hash, uint32_t flag)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.flag = flag;
		}
		else
		{
			assert(false);
		}
	}

	uint32_t GetFlag(const KPositionHashKey& hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.flag;
		}
		assert(false);
		return 0;
	}

	void SetVersion(const KPositionHashKey& hash, int32_t version)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.version = version;
		}
		else
		{
			assert(false);
		}
	}

	void IncVersion(const KPositionHashKey& hash, int32_t inc)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.version += inc;
		}
		else
		{
			assert(false);
		}
	}

	int32_t GetVersion(const KPositionHashKey& hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.version;
		}
		assert(false);
		return -1;
	}

	void SetAdjacency(const KPositionHashKey& hash, const std::unordered_set<size_t>& adjacencies)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.adjacencies = adjacencies;
		}
		else
		{
			assert(false);
		}
	}

	void AddAdjacency(const KPositionHashKey& hash, size_t triangle)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.adjacencies.insert(triangle);
		}
		else
		{
			assert(false);
		}
	}

	void RemoveAdjacency(const KPositionHashKey& hash)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.adjacencies = {};
		}
		else
		{
			assert(false);
		}
	}

	const std::unordered_set<size_t>& GetAdjacency(const KPositionHashKey& hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.adjacencies;
		}
		else
		{
			assert(false);
			static std::unordered_set<size_t> empty;
			return empty;
		}
	}

	size_t GetAdjacencySize(const KPositionHashKey& hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.adjacencies.size();
		}
		assert(false);
		return 0;
	}

	bool HasAdjacency(const KPositionHashKey& hash, size_t triangle)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.adjacencies.find(triangle) != it->second.adjacencies.end();
		}
		else
		{
			assert(false);
			return false;
		}
	}

	KPositionHashKey AddPositionHash(const glm::vec3& position, size_t v)
	{
		KPositionHashKey hash = GetPositionHash(position);
#if ENABLE_POSITION_HASH_KEY_DEBUG
		hash.debug = v;
#endif
		auto it = informations.find(hash);
		if (it == informations.end())
		{
			VertexInfomation newInfo;
			newInfo.vertices = { v };
			informations[hash] = newInfo;
		}
		else
		{
			it->second.vertices.insert(v);
		}
		return hash;
	}

	void RemovePositionHash(const KPositionHashKey& hash, size_t v)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.vertices.erase(v);
		}
	}

	void RemovePositionHashExcept(const KPositionHashKey& hash, size_t v)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			if (it->second.vertices.find(18139) != it->second.vertices.end())
			{
				int x = 0;
			}

			if (it->second.vertices.find(v) != it->second.vertices.end())
			{
				it->second.vertices = { v };
			}
			else
			{
				it->second.vertices = {};
			}
		}
	}

	void RemoveAllPositionHash(const KPositionHashKey& hash)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.vertices = {};
		}
	}

	bool HasPositionHash(const KPositionHashKey& hash) const
	{
		auto it = informations.find(hash);
		return it != informations.end();
	}

	const std::unordered_set<size_t>& GetVertex(const KPositionHashKey& hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.vertices;
		}
		else
		{
			assert(false);
			static std::unordered_set<size_t> empty;
			return empty;
		}
	}

	void ForEachVertex(const KPositionHashKey& hash, std::function<void(size_t)> call)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			for (size_t v : it->second.vertices)
			{
				call(v);
			}
		}
	}

	void ForEachAdjacency(const KPositionHashKey& hash, std::function<void(size_t)> call)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			for (size_t triangle : it->second.adjacencies)
			{
				call(triangle);
			}
		}
	}
};

struct KEdgeHash
{
	// Key为顶点Hash            Key为相邻节点 Value为边所在的三角形列表
	std::unordered_map<KPositionHashKey, std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>> edges;

	KEdgeHash()
	{
		Init();
	}

	void Init()
	{
		edges.clear();
	}

	void UnInit()
	{
		edges.clear();
	}

	void AddEdgeHash(const KPositionHashKey& p0, const KPositionHashKey& p1, size_t triIndex)
	{
		auto itOuter = edges.find(p0);
		if (itOuter == edges.end())
		{
			itOuter = edges.insert({ p0, {} }).first;
		}
		std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>& link = itOuter->second;
		auto it = link.find(p1);
		if (it == link.end())
		{
			it = link.insert({ p1, {} }).first;
		}
		it->second.insert(triIndex);
	}

	void RemoveEdgeHash(const KPositionHashKey& p0, const KPositionHashKey& p1, size_t triIndex)
	{
		auto itOuter = edges.find(p0);
		if (itOuter != edges.end())
		{
			std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>& link = itOuter->second;
			auto it = link.find(p1);
			if (it != link.end())
			{
				it->second.erase(triIndex);
				if (it->second.size() == 0)
				{
					link.erase(p1);
				}
			}
		}
	}

	void ClearEdgeHash(const KPositionHashKey& p0)
	{
		edges.erase(p0);
	}

	void ForEach(const KPositionHashKey& p0, std::function<void(const KPositionHashKey&, size_t)> call)
	{
		auto itOuter = edges.find(p0);
		if (itOuter != edges.end())
		{
			std::unordered_map<KPositionHashKey, std::unordered_set<size_t>> link = itOuter->second;
			for (auto it = link.begin(); it != link.end(); ++it)
			{
				const KPositionHashKey& p1 = it->first;
				std::unordered_set<size_t> tris = it->second;
				for (size_t triIndex : tris)
				{
					call(p1, triIndex);
				}
			}
		}
	}

	bool HasConnection(const KPositionHashKey& p0, const KPositionHashKey& p1) const
	{
		auto itOuter = edges.find(p0);
		if (itOuter != edges.end())
		{
			const std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>& link = itOuter->second;
			return link.find(p1) != link.end();
		}
		return false;
	}

	void ForEachTri(const KPositionHashKey& p0, const KPositionHashKey& p1, std::function<void(size_t)> call)
	{
		auto itOuter = edges.find(p0);
		if (itOuter != edges.end())
		{
			std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>& link = itOuter->second;
			auto it = link.find(p1);
			if (it != link.end())
			{
				for (size_t triIndex : it->second)
				{
					call(triIndex);
				}
			}
		}
	}
};