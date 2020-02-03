#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include <assert.h>

namespace KEnumString
{
	inline const char* ElementForamtToString(ElementFormat format)
	{
#define ENUM(format) case EF_##format: return #format;
		switch (format)
		{
			ENUM(R8GB8BA8_UNORM);
			ENUM(R8G8B8A8_SNORM);
			ENUM(R8GB8B8_UNORM);
			ENUM(R16_FLOAT);
			ENUM(R16G16_FLOAT);
			ENUM(R16G16B16_FLOAT);
			ENUM(R16G16B16A16_FLOAT);
			ENUM(R32_FLOAT);
			ENUM(R32G32_FLOAT);
			ENUM(R32G32B32_FLOAT);
			ENUM(R32G32B32A32_FLOAT);
			ENUM(R32_UINT);
			ENUM(ETC1_R8G8B8_UNORM);
			ENUM(ETC2_R8G8B8_UNORM);
			ENUM(ETC2_R8G8B8A1_UNORM);
			ENUM(ETC2_R8G8B8A8_UNORM);
		default:
			assert(false);
			return "UNKNOWN";
		};
#undef ENUM
	}

	inline ElementFormat StringToElementForamt(const char* str)
	{
#define CMP(enum_string) if (!strcmp(str, #enum_string)) return EF_##enum_string;
		CMP(R8GB8BA8_UNORM);
		CMP(R8G8B8A8_SNORM);
		CMP(R8GB8B8_UNORM);
		CMP(R16_FLOAT);
		CMP(R16G16_FLOAT);
		CMP(R16G16B16_FLOAT);
		CMP(R16G16B16A16_FLOAT);
		CMP(R32_FLOAT);
		CMP(R32G32_FLOAT);
		CMP(R32G32B32_FLOAT);
		CMP(R32G32B32A32_FLOAT);
		CMP(R32_UINT);
		CMP(ETC1_R8G8B8_UNORM);
		CMP(ETC2_R8G8B8_UNORM);
		CMP(ETC2_R8G8B8A1_UNORM);
		CMP(ETC2_R8G8B8A8_UNORM);
		assert(false);
		return EF_UNKNOWN;
#undef CMP
	}
}