#pragma once
#include <memory>
#include <vector>
#include <functional>

#include "KBase/Interface/IKEntity.h"

class KEntity;
typedef std::shared_ptr<KEntity> KEntityPtr;
typedef std::function<void(KEntityPtr&)> KEntityViewFunc;