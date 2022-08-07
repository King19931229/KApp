#include "KHeightMap.h"

KHeightMap::KHeightMap()
	: m_Width(0)
	, m_Height(0)
{}

KHeightMap::~KHeightMap()
{}

void KHeightMap::Init(const uint8_t* data, int32_t width, int32_t height)
{
	UnInit();

	m_Width = width;
	m_Height = height;
	m_HeightData.resize(m_Width * m_Height);

	for (int32_t y = 0; y < m_Height; ++y)
	{
		for (int32_t x = 0; x < m_Width; ++x)
		{
			m_HeightData[y * width + x] = data[y * width + x];
		}
	}
}

void KHeightMap::Init(const char* filename)
{
	UnInit();

	IKCodecPtr pCodec = KCodec::GetCodec(filename);
	KCodecResult imageData;

	if (pCodec && pCodec->Codec(filename, false, imageData))
	{
		ASSERT_RESULT(imageData.uDepth == 1);
		ASSERT_RESULT(imageData.bCompressed == false);
		ASSERT_RESULT(imageData.bCubemap == false);
		ASSERT_RESULT(imageData.b3DTexture == false);

		m_Width = (int32_t)imageData.uWidth;
		m_Height = (int32_t)imageData.uHeight;
		m_HeightData.resize(m_Width * m_Height);

		unsigned char* srcData = imageData.pData->GetData();

		for (int32_t y = 0; y < m_Width; ++y)
		{
			for (int32_t x = 0; x < m_Height; ++x)
			{
				if (imageData.eFormat == IF_R32_FLOAT)
				{
					m_HeightData[y * m_Width + x] = ((float*)srcData)[y * m_Width + x];
				}
				else if (imageData.eFormat == IF_R8)
				{
					m_HeightData[y * m_Width + x] = ((uint8_t*)srcData)[y * m_Width + x];
				}
				else if (imageData.eFormat == IF_R8G8B8A8)
				{
					uint8_t r = ((uint8_t*)srcData)[(y * m_Width + x) * 4 + 0];
					uint8_t g = ((uint8_t*)srcData)[(y * m_Width + x) * 4 + 1];
					uint8_t b = ((uint8_t*)srcData)[(y * m_Width + x) * 4 + 2];
					uint16_t h = (r * 30 + g * 59 + b * 11) / 100;
					m_HeightData[y * m_Width + x] = h;
				}
				else if (imageData.eFormat == IF_R8G8B8)
				{
					uint8_t r = ((uint8_t*)srcData)[(y * m_Width + x) * 3 + 0];
					uint8_t g = ((uint8_t*)srcData)[(y * m_Width + x) * 3 + 1];
					uint8_t b = ((uint8_t*)srcData)[(y * m_Width + x) * 3 + 2];
					m_HeightData[y * m_Width + x] = (float)((r * 30 + g * 59 + b * 11) / 100);
				}
				else if (imageData.eFormat == IF_R32G32B32A32_FLOAT)
				{
					float r = ((float*)srcData)[(y * m_Width + x) * 4 + 0];
					float g = ((float*)srcData)[(y * m_Width + x) * 4 + 1];
					float b = ((float*)srcData)[(y * m_Width + x) * 4 + 2];
					m_HeightData[y * m_Width + x] = r * 0.3f + g * 0.59f + b * 0.11f;
				}
				else if (imageData.eFormat == IF_R32G32B32_FLOAT)
				{
					float r = ((float*)srcData)[(y * m_Width + x) * 3 + 0];
					float g = ((float*)srcData)[(y * m_Width + x) * 3 + 1];
					float b = ((float*)srcData)[(y * m_Width + x) * 3 + 2];
					m_HeightData[y * m_Width + x] = r * 0.3f + g * 0.59f + b * 0.11f;
				}
				else
				{
					ASSERT_RESULT(false && "should not reach");
				}
			}
		}
	}
}

void KHeightMap::UnInit()
{
	m_Width = 0;
	m_Height = 0;
	m_HeightData.clear();
}

float KHeightMap::GetData(int32_t x, int32_t y) const
{
	if (x >= m_Width || y >= m_Height || x < 0 || y < 0)
		return 0;
	return m_HeightData[y * m_Width + x];
}

float KHeightMap::GetData(float u, float v) const
{
	if (u < 0 || u > 1 || v < 0 || v > 1)
		return 0;

	float x = u * (m_Width - 1);
	float y = v * (m_Height - 1);

	int32_t x_floor = (int32_t)floor(x);
	int32_t y_floor = (int32_t)floor(y);

	int32_t x_ceil = (int32_t)ceil(x);
	int32_t y_ceil = (int32_t)ceil(y);

	float s = x - x_floor;
	float t = y - y_floor;

	float h0 = m_HeightData[y_floor * m_Width + x_floor];
	float h1 = m_HeightData[y_floor * m_Width + x_ceil];
	float h2 = m_HeightData[y_ceil * m_Width + x_floor];
	float h3 = m_HeightData[y_ceil * m_Width + x_ceil];

	float h = (h0 * (1 - s) + h1 * s) * (1 - t) + (h2 * (1 - s) + h3 * s) * t;
	return h;
}