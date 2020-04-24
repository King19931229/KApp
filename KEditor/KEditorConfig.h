#pragma once
#include "KBase/Publish/KConfig.h"
#include "KBase/Interface/Entity/IKEntity.h"

struct KEEntityCreateInfo
{
	std::string path;
	bool isMesh;

	KEEntityCreateInfo()
	{
		isMesh = true;
	}
};

struct KEEntity
{
	IKEntityPtr soul;	
	KEEntityCreateInfo createInfo;

	bool operator<(const KEEntity& rhs) const
	{
		return soul < rhs.soul;
	}

	bool operator==(const KEEntity& rhs) const
	{
		return soul == rhs.soul;
	}
};

namespace std
{
	template<>
	struct hash<KEEntity>
	{
		inline std::size_t operator()(const KEEntity& entity) const
		{
			return (size_t)(entity.soul.get());
		}
	};
}