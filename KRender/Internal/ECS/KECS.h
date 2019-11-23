#pragma once
#include <memory>
#include <vector>
#include <functional>

enum ComponentType
{
	CT_TRANSFORM,
	CT_RENDER,

	CT_COUNT,
	CT_UNKNOWN = CT_COUNT
};

typedef std::vector<ComponentType> ComponentTypeList;

class KEntity;
typedef std::shared_ptr<KEntity> KEntityPtr;
typedef std::function<void(KEntityPtr&)> KEntityViewFunc;

class KComponentBase;