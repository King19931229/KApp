#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace KInteger
{
	inline bool FitsUInt32(uint64_t value)
	{
		return value <= (std::numeric_limits<uint32_t>::max)();
	}

	inline bool FitsMemorySize(uint64_t value)
	{
		return value <= (std::numeric_limits<size_t>::max)();
	}
}
