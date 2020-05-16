#pragma once
#include "rttr/registration.h"
#include "rttr/registration_friend.h"
#include "rttr/type.h"
#include <vector>

#define META_DATA_TYPE "metadata_type"
#define META_DATA_NOTIFY "metadata_notify"

enum MetaDataType
{
	MDT_INT,
	MDT_FLOAT,
	MDT_STDSTRING,

	MDT_FLOAT2,
	MDT_FLOAT3,
	MDT_FLOAT4,

	MDT_OBJECT,

	MDT_UNKNOWN
};

enum MetaDataNotify
{
	MDN_NONE,
	MDN_EDITOR,
};

struct KReflectionObjectBase
{
	RTTR_ENABLE()
	RTTR_REGISTRATION_FRIEND
public:
	KReflectionObjectBase() {}
	~KReflectionObjectBase() {}
};

#define KRTTR_GET_TYPE(obj) rttr::type::get(*obj)

#define KRTTR_REG_CLASS_BEGIN rttr::registration::class_<KRTTR_REG_CLASS_NAME>(KRTTR_REG_CLASS_NAME_STR).constructor<>

#define KRTTR_REG_METADATA rttr::metadata

#define KRTTR_REG_PROPERTY(property_name, data_type) .property(#property_name, &KRTTR_REG_CLASS_NAME::##property_name)(KRTTR_REG_METADATA(META_DATA_TYPE, data_type))

#define KRTTR_REG_PROPERTY_GET_SET_NOTIFY(property_name, getter, setter, data_type, notify_type) .property(#property_name, &KRTTR_REG_CLASS_NAME::##getter, &KRTTR_REG_CLASS_NAME::##setter)(KRTTR_REG_METADATA(META_DATA_TYPE, data_type), KRTTR_REG_METADATA(META_DATA_NOTIFY, notify_type))
#define KRTTR_REG_PROPERTY_GET_SET(property_name, getter, setter, data_type) KRTTR_REG_PROPERTY_GET_SET_NOTIFY(property_name, getter, setter, data_type, MDN_NONE)

#define KRTTR_REG_PROPERTY_READ_ONLY(property_name, getter, data_type) .property_readonly(#property_name, &KRTTR_REG_CLASS_NAME::##getter)(KRTTR_REG_METADATA(META_DATA_TYPE, data_type))

#define KRTTR_REG_METHOD(prototype, method_name) .method(#method_name, rttr::select_overload<prototype>(&KRTTR_REG_CLASS_NAME::##method_name))

#define KRTTR_REG_CLASS_END() ;