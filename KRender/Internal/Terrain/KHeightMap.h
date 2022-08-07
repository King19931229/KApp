#pragma once
#include "KBase/Interface/IKCodec.h"

class KHeightMap
{
protected:
	int32_t m_Width;
	int32_t m_Height;
	std::vector<float> m_HeightData;
public:
	KHeightMap();
	~KHeightMap();

	void Init(const uint8_t* data, int32_t width, int32_t height);
	void Init(const char* filename);
	void UnInit();

	float GetData(int32_t x, int32_t y) const;
	float GetData(float u, float v) const;

	int32_t GetWidth() const { return m_Width; }
	int32_t GetHeight() const { return m_Height; }
};