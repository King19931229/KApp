#pragma once
#include "rttr/registration.h"
#include "rttr/type.h"

#define KRTTR_REG_CLASS_BEGIN() rttr::registration::class_<KRTTR_REG_CLASS_NAME>(KRTTR_REG_CLASS_NAME_STR).constructor<>()
#define KRTTR_REG_PROPERTY(property_name) .property(#property_name, &KRTTR_REG_CLASS_NAME::##property_name)
#define KRTTR_REG_METHOD(prototype, method_name) .method(#method_name, rttr::select_overload<prototype>(&KRTTR_REG_CLASS_NAME::##method_name))
#define KRTTR_REG_CLASS_END() ;