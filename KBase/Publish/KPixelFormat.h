#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

enum PixelFormat
{
#define PIXEL_FORMAT(format, width, height, bytes, compression, srgb) PF_##format,
#include "KPixelFormat.inl"
#undef PIXEL_FORMAT

	PF_UNKNOWN,
	PF_COUNT = PF_UNKNOWN
};

namespace KPixelFormatHelper
{
	enum class Compression
	{
		NONE,
		BC,
		ETC1,
		ETC2,
		ASTC
	};

	struct FormatInfo
	{
		const char* name;
		uint32_t blockWidth;
		uint32_t blockHeight;
		uint32_t blockBytes;
		Compression compression;
		bool srgb;
	};

	inline bool IsValid(PixelFormat format)
	{
		return format >= 0 && format < PF_COUNT;
	}

	inline const FormatInfo* GetInfo(PixelFormat format)
	{
		static const FormatInfo formats[] =
		{
#define PIXEL_FORMAT(format, width, height, bytes, compression, srgb) \
			{ #format, width, height, bytes, Compression::compression, srgb },
#include "KPixelFormat.inl"
#undef PIXEL_FORMAT
		};
		static_assert(sizeof(formats) / sizeof(formats[0]) == PF_COUNT, "Incomplete pixel format table");
		return IsValid(format) ? &formats[format] : nullptr;
	}

	inline bool IsCompressed(PixelFormat format)
	{
		const FormatInfo* info = GetInfo(format);
		return info && info->compression != Compression::NONE;
	}

	inline bool IsSRGBFormat(PixelFormat format)
	{
		const FormatInfo* info = GetInfo(format);
		return info && info->srgb;
	}

	inline bool IsBCFormat(PixelFormat format)
	{
		const FormatInfo* info = GetInfo(format);
		return info && info->compression == Compression::BC;
	}

	inline bool IsETC1Format(PixelFormat format)
	{
		const FormatInfo* info = GetInfo(format);
		return info && info->compression == Compression::ETC1;
	}

	inline bool IsETC2Format(PixelFormat format)
	{
		const FormatInfo* info = GetInfo(format);
		return info && info->compression == Compression::ETC2;
	}

	inline bool IsASTCFormat(PixelFormat format)
	{
		const FormatInfo* info = GetInfo(format);
		return info && info->compression == Compression::ASTC;
	}

	inline bool GetElementByteSize(PixelFormat format, size_t& size)
	{
		size = 0;
		const FormatInfo* info = GetInfo(format);
		if (!info || info->compression != Compression::NONE) return false;
		size = info->blockBytes;
		return true;
	}

	// Uncompressed formats use a single pixel as their storage block.
	inline bool GetBlockByteSize(PixelFormat format, size_t& size)
	{
		size = 0;
		const FormatInfo* info = GetInfo(format);
		if (!info) return false;
		size = info->blockBytes;
		return true;
	}

	inline bool GetBlockDimension(PixelFormat format, uint32_t& width, uint32_t& height)
	{
		width = height = 0;
		const FormatInfo* info = GetInfo(format);
		if (!info) return false;
		width = info->blockWidth;
		height = info->blockHeight;
		return true;
	}

	namespace Detail
	{
		inline bool Multiply(size_t left, size_t right, size_t& result)
		{
			if (left && right > (std::numeric_limits<size_t>::max)() / left) return false;
			result = left * right;
			return true;
		}
	}

	inline bool GetByteSize(PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, size_t& size)
	{
		size = 0;
		const FormatInfo* info = GetInfo(format);
		if (!info || !width || !height || !depth) return false;

		const size_t blocksX = width / info->blockWidth + (width % info->blockWidth != 0);
		const size_t blocksY = height / info->blockHeight + (height % info->blockHeight != 0);
		size_t bytes = 0;
		if (!Detail::Multiply(blocksX, blocksY, bytes)
			|| !Detail::Multiply(bytes, depth, bytes)
			|| !Detail::Multiply(bytes, info->blockBytes, bytes)) return false;
		size = bytes;
		return true;
	}

	// faces can also specify the number of array layers.
	inline bool GetByteSize(PixelFormat format, uint32_t mipmaps, uint32_t faces,
		uint32_t width, uint32_t height, uint32_t depth, size_t& totalSize)
	{
		totalSize = 0;
		if (!mipmaps || !faces) return false;
		size_t total = 0;
		for (size_t mip = 0; mip < mipmaps; ++mip)
		{
			size_t bytes = 0;
			if (!GetByteSize(format, width, height, depth, bytes)
				|| !Detail::Multiply(bytes, faces, bytes)
				|| bytes > (std::numeric_limits<size_t>::max)() - total) return false;
			total += bytes;
			if (width > 1) width >>= 1;
			if (height > 1) height >>= 1;
			if (depth > 1) depth >>= 1;
		}
		totalSize = total;
		return true;
	}

	inline const char* ToString(PixelFormat format)
	{
		const FormatInfo* info = GetInfo(format);
		return info ? info->name : "UNKNOWN";
	}

	inline PixelFormat FromString(const char* name)
	{
		if (name)
		{
			for (int format = 0; format < PF_COUNT; ++format)
			{
				if (std::strcmp(name, GetInfo(PixelFormat(format))->name) == 0) return PixelFormat(format);
			}
		}
		return PF_UNKNOWN;
	}
}
