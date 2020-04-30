#pragma once
#include "KBase/Publish/KConfig.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KEngine/Interface/IKEngine.h"

struct KEEntityCreateInfo
{
	std::string path;
	glm::mat4 transform;

	KEEntityCreateInfo()
	{
		transform = glm::mat4(1.0f);
	}
};

struct KEEntityTransformInfo
{
	glm::mat4 previous;
	glm::mat4 current;

	KEEntityTransformInfo()
	{
		previous = current = glm::mat4(1.0f);
	}
};

struct KEEntity
{
	IKEntityPtr soul;	
	// for create or delete undo(redo)
	KEEntityCreateInfo createInfo;
	// for transform undo(redo)
	KEEntityTransformInfo transformInfo;

	KEEntity()
	{
		soul = nullptr;
	}

	~KEEntity()
	{
		if (soul)
		{
			IKEnginePtr engine = KEngineGlobal::Engine;
			if (engine)
			{
				engine->Wait();
			}

			soul->UnRegisterAllComponent();
			if (KECS::EntityManager)
			{
				KECS::EntityManager->ReleaseEntity(soul);
			}
		}
	}

	bool operator<(const KEEntity& rhs) const
	{
		return soul < rhs.soul;
	}

	bool operator==(const KEEntity& rhs) const
	{
		return soul == rhs.soul;
	}

	bool SetSelect(bool select)
	{
		// TODO
	}
};

typedef std::shared_ptr<KEEntity> KEEntityPtr;

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