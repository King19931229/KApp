#pragma once
#include <memory>
#include <vector>
#include <functional>

enum ComponentType
{
	CT_TRANSFORM,
	CT_RENDER,
	CT_CULL,

	CT_COUNT,
	CT_UNKNOWN = CT_COUNT
};

typedef std::vector<ComponentType> ComponentTypeList;

class KComponentBase;
typedef std::shared_ptr<KComponentBase> KComponentBasePtr;

class KEntity;
typedef std::shared_ptr<KEntity> KEntityPtr;
//typedef std::vector<KEntityPtr> KEntityPtrList;

typedef std::function<void(KEntityPtr&)> KEntityViewFunc;

class KSystemBase;
typedef std::shared_ptr<KSystemBase> KSystemBasePtr;
//typedef std::vector<KSystemBasePtr> KSystemBasePtrList;