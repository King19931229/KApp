#include "KTransformComponent.h"

RTTR_REGISTRATION
{
#define KRTTR_REG_CLASS_NAME KTransformComponent
#define KRTTR_REG_CLASS_NAME_STR "TransformComponent"

	KRTTR_REG_CLASS_BEGIN()
	KRTTR_REG_PROPERTY_GET_SET("position", GetPosition, SetPosition, MDT_FLOAT3)
	KRTTR_REG_PROPERTY_GET_SET("scale", GetScale, SetScale, MDT_FLOAT3)
	KRTTR_REG_CLASS_END()

#undef KRTTR_REG_CLASS_NAME_STR
#undef KRTTR_REG_CLASS_NAME
}