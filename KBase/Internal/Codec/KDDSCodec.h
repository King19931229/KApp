#pragma once
#include "Internal/KCodec.h"
#include "Interface/IKDataStream.h"

struct ColourValue;
struct DXTColourBlock;
struct DXTExplicitAlphaBlock;
struct DXTInterpolatedAlphaBlock;

class KDDSCodec : public IKCodec
{
public:
	KDDSCodec();
	virtual ~KDDSCodec();

	void FlipEndian(void *pData, size_t size, size_t count) const;
	void FlipEndian(void *pData, size_t size) const;


	ImageFormat ConvertFourCCFormat(uint32_t fourcc) const;
	ImageFormat SearchImageFormat(uint32_t rgbBits, uint32_t rMask, uint32_t gMask, uint32_t bMask, uint32_t aMask) const;

	// Unpack R5G6B5 colours into array of 16 colour values
	void UnpackR5G6B5Colour(const void* src, ColourValue *pCol) const;
	// Unpack DXT colours into array of 16 colour values
	void UnpackDXTColour(ImageFormat foramt, const DXTColourBlock &block, ColourValue *pCol) const;
	// Unpack DXT alphas into array of 16 colour values
	void UnpackDXTAlpha(const DXTExplicitAlphaBlock &block, ColourValue *pCol) const;
	// Unpack DXT alphas into array of 16 colour values
	void UnpackDXTAlpha(const DXTInterpolatedAlphaBlock &block, ColourValue *pCol) const;

	virtual bool Codec(const char* pszFile, bool forceAlpha, KCodecResult& result);
	virtual bool Save(const KCodecResult& source, const char* pszFile);

	static bool Init();
	static bool UnInit();
};