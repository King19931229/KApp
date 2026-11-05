#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/IKJson.h"
#include "KBase/Interface/IKLog.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
	unsigned checks = 0;
#define CHECK(expression) do { ++checks; if (!(expression)) throw std::runtime_error( \
	std::string(#expression) + " at line " + std::to_string(__LINE__)); } while (false)

	void TestSizes()
	{
		const struct { PixelFormat format; size_t bytes; } elements[] =
		{
			{ PF_R8_UNORM, 1 }, { PF_R8G8_UNORM, 2 }, { PF_R8G8B8_UNORM, 3 },
			{ PF_R8G8B8A8_UNORM, 4 }, { PF_R8G8B8A8_SNORM, 4 },
			{ PF_R4G4B4A4_UNORM, 2 }, { PF_R5G5B5A1_UNORM, 2 },
			{ PF_R16_FLOAT, 2 }, { PF_R16G16_FLOAT, 4 }, { PF_R16G16B16_FLOAT, 6 },
			{ PF_R16G16B16A16_FLOAT, 8 }, { PF_R32_FLOAT, 4 }, { PF_R32G32_FLOAT, 8 },
			{ PF_R32G32B32_FLOAT, 12 }, { PF_R32G32B32A32_FLOAT, 16 },
			{ PF_R16_UINT, 2 }, { PF_R32_UINT, 4 }, { PF_R32_INT, 4 },
			{ PF_R32G32B32A32_UINT, 16 }, { PF_R32G32B32A32_INT, 16 },
			{ PF_R8G8B8_SRGB, 3 }, { PF_R8G8B8A8_SRGB, 4 }
		};
		for (const auto& element : elements)
		{
			size_t bytes = 0;
			CHECK(KPixelFormatHelper::GetElementByteSize(element.format, bytes));
			CHECK(bytes == element.bytes);
			CHECK(KPixelFormatHelper::GetByteSize(element.format, 3, 5, 7, bytes));
			CHECK(bytes == 105 * element.bytes);
		}
		size_t bytes = 0;
		CHECK(KPixelFormatHelper::GetByteSize(PF_R8G8B8A8_UNORM, 4, 4, 4, bytes) && bytes == 256);
		CHECK(KPixelFormatHelper::GetByteSize(PF_R8G8B8A8_UNORM, 3, 1, 4, 4, 4, bytes) && bytes == 292);
		CHECK(KPixelFormatHelper::GetByteSize(PF_R8G8B8A8_UNORM, 3, 6, 4, 4, 1, bytes) && bytes == 504);
		CHECK(KPixelFormatHelper::GetByteSize(PF_R8G8B8A8_UNORM, 1, 3, 4, 4, 1, bytes) && bytes == 192);
		CHECK(KPixelFormatHelper::GetByteSize(PF_BC1_RGBA_UNORM, 5, 7, 2, bytes) && bytes == 64);
		CHECK(KPixelFormatHelper::GetByteSize(PF_BC2_SRGB, 1, 1, 1, bytes) && bytes == 16);
		CHECK(KPixelFormatHelper::GetByteSize(PF_ETC2_R8G8B8A8_UNORM, 1, 1, 1, bytes) && bytes == 16);
		CHECK(KPixelFormatHelper::GetByteSize(PF_ASTC_5x4_SRGB, 11, 9, 1, bytes) && bytes == 144);
		CHECK(KPixelFormatHelper::GetByteSize(PF_ASTC_12x12_UNORM, 1, 1, 1, bytes) && bytes == 16);
		CHECK(KPixelFormatHelper::GetByteSize(PF_BC1_RGBA_UNORM, 16777217, 1, 1, bytes) && bytes == 33554440);
		CHECK(!KPixelFormatHelper::GetElementByteSize(PF_BC1_RGBA_UNORM, bytes) && bytes == 0);
		const size_t maximum = (std::numeric_limits<size_t>::max)();
		CHECK(KPixelFormatHelper::GetByteSize(PF_R8_UNORM, maximum, 1, 1, bytes) && bytes == maximum);
		CHECK(!KPixelFormatHelper::GetByteSize(PF_R8G8B8A8_UNORM, maximum, 1, 1, bytes) && bytes == 0);
		CHECK(!KPixelFormatHelper::GetByteSize(PF_BC1_RGBA_UNORM, maximum, 1, 1, bytes) && bytes == 0);
		CHECK(!KPixelFormatHelper::GetByteSize(PF_R8_UNORM, 2, 1, maximum, 1, 1, bytes) && bytes == 0);
		CHECK(!KPixelFormatHelper::GetByteSize(PF_R8G8B8A8_UNORM, 1, maximum, 1, 1, 1, bytes) && bytes == 0);
		CHECK(!KPixelFormatHelper::GetByteSize(PF_UNKNOWN, 1, 1, 1, bytes) && bytes == 0);
		CHECK(!KPixelFormatHelper::GetByteSize(PF_R8_UNORM, 0, 1, 1, bytes) && bytes == 0);
		CHECK(!KPixelFormatHelper::GetByteSize(PF_R8_UNORM, 0, 1, 1, 1, 1, bytes) && bytes == 0);
		CHECK(!KPixelFormatHelper::GetByteSize(PF_R8_UNORM, 1, 0, 1, 1, 1, bytes) && bytes == 0);
		CHECK(!KPixelFormatHelper::IsValid(static_cast<PixelFormat>(-1)));
		CHECK(!KPixelFormatHelper::IsValid(static_cast<PixelFormat>(PF_COUNT + 1)));
	}

	void TestNamesAndResources()
	{
		for (int value = 0; value < PF_COUNT; ++value)
		{
			const PixelFormat format = static_cast<PixelFormat>(value);
			CHECK(KPixelFormatHelper::FromString(KPixelFormatHelper::ToString(format)) == format);
		}
		CHECK(KPixelFormatHelper::FromString(nullptr) == PF_UNKNOWN);
		CHECK(KPixelFormatHelper::FromString("UNKNOWN") == PF_UNKNOWN);
		CHECK(KPixelFormatHelper::FromString("invalid") == PF_UNKNOWN);
		CHECK(KPixelFormatHelper::IsBCFormat(PF_BC1_RGB_SRGB));
		CHECK(KPixelFormatHelper::IsETC1Format(PF_ETC1_R8G8B8_UNORM));
		CHECK(KPixelFormatHelper::IsETC2Format(PF_ETC2_R8G8B8A1_UNORM));
		CHECK(KPixelFormatHelper::IsASTCFormat(PF_ASTC_10x6_SRGB));
		CHECK(!KPixelFormatHelper::IsCompressed(PF_R32_UINT));
		CHECK(!KPixelFormatHelper::IsSRGBFormat(PF_UNKNOWN));
		// Representative integers from existing post-process JSON resources.
		const struct { int value; PixelFormat format; } resources[] =
		{
			{ 0, PF_R8G8B8A8_UNORM }, { 6, PF_R8_UNORM }, { 13, PF_R32G32B32_FLOAT },
			{ 27, PF_BC1_RGBA_SRGB }, { 40, PF_ASTC_4x4_UNORM }, { 67, PF_ASTC_12x12_SRGB }
		};
		for (const auto& resource : resources)
		{
			IKJsonDocumentPtr json = GetJsonDocument();
			CHECK(json->ParseFromString(("{\"format\":" + std::to_string(resource.value) + "}").c_str()));
			CHECK(static_cast<PixelFormat>(json->GetMember("format")->GetInt()) == resource.format);
		}
	}

	void Append32(std::vector<unsigned char>& data, uint32_t value)
	{
		for (unsigned byte = 0; byte < 4; ++byte) data.push_back(static_cast<unsigned char>(value >> (8 * byte)));
	}

	std::vector<unsigned char> MakeDDS(uint32_t dxgi, unsigned family, uint32_t width,
		uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t legacy = 0, unsigned pixelBytes = 4)
	{
		std::vector<unsigned char> data;
		Append32(data, 0x20534444); // DDS magic
		uint32_t header[31] = {};
		header[0] = 124;
		header[1] = 0x1007 | (family ? 0x80000 : 0x8) | (depth > 1 ? 0x800000 : 0) | (mipmaps > 1 ? 0x20000 : 0);
		header[2] = height;
		header[3] = width;
		header[4] = family ? ((width + 3) / 4) * ((height + 3) / 4) * (family == 1 ? 8 : 16) : width * pixelBytes;
		header[5] = depth;
		header[6] = mipmaps;
		header[18] = 32;
		header[19] = 4; // FourCC
		header[20] = legacy ? legacy : 0x30315844; // DX10
		header[26] = 0x1000 | (mipmaps > 1 ? 0x400008 : 0);
		header[27] = depth > 1 ? 0x200000 : 0;
		for (uint32_t field : header) Append32(data, field);
		if (!legacy)
		{
			Append32(data, dxgi);
			Append32(data, depth > 1 ? 4 : 3);
			Append32(data, 0);
			Append32(data, 1);
			Append32(data, 0);
		}
		for (uint32_t mip = 0; mip < mipmaps; ++mip)
		{
			if (family)
			{
				const size_t blocks = size_t((width + 3) / 4) * ((height + 3) / 4) * depth;
				for (size_t block = 0; block < blocks; ++block)
				{
					if (family == 2) data.insert(data.end(), 8, 255);
					if (family == 3)
					{
						data.push_back(255);
						data.insert(data.end(), 7, 0);
					}
					Append32(data, 0x00008000); // red = 16/31, second endpoint = black
					Append32(data, 0); // select first endpoint
				}
			}
			else
			{
				for (size_t byte = 0; byte < size_t(width) * height * depth * pixelBytes; ++byte)
					data.push_back(static_cast<unsigned char>(byte % 251));
			}
			width = std::max(1u, width / 2);
			height = std::max(1u, height / 2);
			depth = std::max(1u, depth / 2);
		}
		return data;
	}

	KCodecResult DecodeDDS(const std::vector<unsigned char>& data, bool hardware, bool forceAlpha)
	{
		KCodec::BCHardwareCodec = hardware;
		IKCodecPtr codec = KCodec::GetCodec("sample.dds");
		CHECK(codec != nullptr);
		KCodecResult result;
		CHECK(codec->Codec(reinterpret_cast<const char*>(data.data()), data.size(), forceAlpha, result));
		size_t expectedSize = 0;
		CHECK(KPixelFormatHelper::GetByteSize(result.eFormat, result.uMipmap, 1,
			result.uWidth, result.uHeight, result.uDepth, expectedSize));
		CHECK(result.pData->GetSize() == expectedSize);
		size_t offset = 0;
		for (const KSubImageInfo& image : result.pData->GetSubImageInfo())
		{
			CHECK(image.uOffset == offset);
			offset += image.uSize;
			CHECK(offset <= expectedSize);
		}
		CHECK(offset == expectedSize);
		return result;
	}

	void CheckDecodedPixels(const KCodecResult& result)
	{
		size_t channels = 0;
		CHECK(!result.bCompressed);
		CHECK(KPixelFormatHelper::GetElementByteSize(result.eFormat, channels));
		for (size_t offset = 0; offset < result.pData->GetSize(); offset += channels)
		{
			const unsigned char* pixel = result.pData->GetData() + offset;
			CHECK(pixel[0] == 131 && pixel[1] == 0 && pixel[2] == 0);
			if (channels == 4) CHECK(pixel[3] == 255);
		}
	}

	void TestDDS(const char* pngPath)
	{
		const struct { uint32_t dxgi; unsigned family; PixelFormat format; bool srgb; } formats[] =
		{
			{ 71, 1, PF_BC1_RGBA_UNORM, false }, { 72, 1, PF_BC1_RGBA_SRGB, true },
			{ 74, 2, PF_BC2_UNORM, false }, { 75, 2, PF_BC2_SRGB, true },
			{ 77, 3, PF_BC3_UNORM, false }, { 78, 3, PF_BC3_SRGB, true }
		};
		for (const auto& format : formats)
		{
			const auto data = MakeDDS(format.dxgi, format.family, 5, 3, 1, 2);
			KCodecResult compressed = DecodeDDS(data, true, true);
			CHECK(compressed.eFormat == format.format && compressed.bCompressed);
			CHECK(std::memcmp(compressed.pData->GetData(), data.data() + 148, compressed.pData->GetSize()) == 0);
			CHECK(KPixelFormatHelper::IsSRGBFormat(compressed.eFormat) == format.srgb);
			KCodecResult decoded = DecodeDDS(data, false, true);
			CHECK(decoded.eFormat == (format.srgb ? PF_R8G8B8A8_SRGB : PF_R8G8B8A8_UNORM));
			CheckDecodedPixels(decoded);
			if (format.family == 1)
			{
				KCodecResult rgb = DecodeDDS(data, false, false);
				CHECK(rgb.eFormat == (format.srgb ? PF_R8G8B8_SRGB : PF_R8G8B8_UNORM));
				CheckDecodedPixels(rgb);
			}
			KCodecResult singleMip = DecodeDDS(MakeDDS(format.dxgi, format.family, 4, 4, 1, 1), true, true);
			CHECK(KPixelFormatHelper::IsSRGBFormat(singleMip.eFormat) == format.srgb);
			CheckDecodedPixels(singleMip);
		}
		for (unsigned dxt = 1; dxt <= 5; ++dxt)
		{
			const unsigned family = dxt == 1 ? 1 : (dxt <= 3 ? 2 : 3);
			const uint32_t fourCC = 0x00545844 | (uint32_t('0' + dxt) << 24);
			const auto data = MakeDDS(0, family, 4, 4, 1, 2, fourCC);
			KCodecResult compressed = DecodeDDS(data, true, true);
			CHECK(compressed.eFormat == (family == 1 ? PF_BC1_RGBA_UNORM : (family == 2 ? PF_BC2_UNORM : PF_BC3_UNORM)));
			CheckDecodedPixels(DecodeDDS(data, false, true));
		}
		for (uint32_t dxgi : { 28u, 29u })
		{
			const auto data = MakeDDS(dxgi, 0, 8, 8, 8, 4);
			KCodecResult volume = DecodeDDS(data, true, true);
			CHECK(volume.eFormat == (dxgi == 29 ? PF_R8G8B8A8_SRGB : PF_R8G8B8A8_UNORM));
			CHECK(volume.b3DTexture && volume.pData->GetSize() == 2340);
			CHECK(std::memcmp(volume.pData->GetData(), data.data() + 148, 2340) == 0);
		}
		KCodecResult decodedVolume = DecodeDDS(MakeDDS(72, 1, 5, 3, 3, 2), false, true);
		CHECK(decodedVolume.eFormat == PF_R8G8B8A8_SRGB && decodedVolume.pData->GetSize() == 188);
		CheckDecodedPixels(decodedVolume);
		for (const auto& format : { std::make_pair(54u, 2u), std::make_pair(41u, 4u), std::make_pair(34u, 4u) })
		{
			KCodecResult image = DecodeDDS(MakeDDS(format.first, 0, 4, 4, 1, 1, 0, format.second), true, true);
			CHECK(image.pData->GetSize() == 16 * format.second);
		}
		KCodecResult rgba = DecodeDDS(MakeDDS(75, 2, 4, 4, 1, 1), false, true);
		IKCodecPtr png = KCodec::GetCodec("sample.png");
		CHECK(png && png->Save(rgba, pngPath));
		CHECK(std::remove(pngPath) == 0);
	}
}

int main(int argc, char** argv)
{
	bool success = false;
	KLog::CreateLogger();
	try
	{
		CHECK(KCodec::CreateCodecManager());
		TestSizes();
		TestNamesAndResources();
		TestDDS(argc > 1 ? argv[1] : "pixel-format-srgb-test.png");
		success = true;
		std::printf("Pixel format regression tests passed (%u checks).\n", checks);
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "Pixel format regression failed: %s\n", error.what());
	}
	KCodec::DestroyCodecManager();
	KLog::DestroyLogger();
	return success ? 0 : 1;
}
