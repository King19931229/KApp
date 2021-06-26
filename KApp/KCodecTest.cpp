#define MEMORY_DUMP_DEBUG
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKLog.h"

int main()
{
	KLog::CreateLogger();
	KCodec::CreateCodecManager();
	KFileSystem::CreateFileManager();

	IKFileSystemPtr sys = KFileSystem::CreateFileSystem(FST_NATIVE);
	sys->Init();
	KFileSystem::Manager->SetFileSystem(FSD_RESOURCE, sys);

	IKCodecPtr codec = KCodec::GetCodec("C:/Users/Admin/Desktop/source2.png");
	KCodecResult input;
	if (codec->Codec("C:/Users/Admin/Desktop/source.png", true, input))
	{
		KCodecResult output = input;
		int grap = 1;

		output.uWidth = (output.uWidth + grap - 1) / grap;
		output.uHeight = (output.uHeight + grap - 1) / grap;

		size_t destImageSize = 0;
		KImageHelper::GetByteSize(output.eFormat, output.uWidth, output.uHeight, output.uDepth, destImageSize);
		size_t elementSize = 0;
		KImageHelper::GetElementByteSize(input.eFormat, elementSize);

		unsigned char* sourceImage = input.pData->GetData();
		size_t soucePitch = elementSize * input.uWidth;

		struct SampleRes
		{
			unsigned char r, g, b, a;
		};

		auto Sample = [sourceImage, soucePitch, elementSize](size_t x, size_t y)
		{
			unsigned char* data = (unsigned char*)((size_t)sourceImage + y * soucePitch + x * elementSize);
			SampleRes res;
			res.r = data[0];
			res.g = data[1];
			res.b = data[2];
			res.a = data[3];
			return res;
		};

		output.pData = KImageDataPtr(KNEW KImageData(destImageSize));
		output.pData->GetSubImageInfo().push_back(KSubImageInfo());

		unsigned char* destImage = output.pData->GetData();

		int** distanceMap = KNEW int* [output.uWidth];
		for (size_t i = 0; i < output.uWidth; ++i)
		{
			distanceMap[i] = KNEW int[output.uHeight];
			memset(distanceMap[i], 0, sizeof(int) * output.uHeight);
		}

		int maxSearchDis = 32;

		int maxValue = std::numeric_limits<int>::min();
		int minValue = std::numeric_limits<int>::max();

		for (int dx = 0; dx < (int)output.uWidth; ++dx)
		{
			for (int dy = 0; dy < (int)output.uHeight; ++dy)
			{
				int sx = dx * grap;
				int sy = dy * grap;

				SampleRes refSample = Sample(sx, sy);
				int ref = (refSample.r + refSample.g + refSample.b) == 765 ? 1 : 0;

				int startX = (sx >= maxSearchDis) ? (sx - maxSearchDis) : 0;
				int startY = (sy >= maxSearchDis) ? (sy - maxSearchDis) : 0;
				int endX = (sx + maxSearchDis < input.uWidth) ? (sx + maxSearchDis) : (int)(input.uWidth - 1);
				int endY = (sy + maxSearchDis < input.uHeight) ? (sy + maxSearchDis) : (int)(input.uHeight - 1);

				// MAX
				int distance = maxSearchDis * maxSearchDis / 4;

				for (int x = startX; x <= endX; ++x)
				{
					for (int y = startY; y <= endY; ++y)
					{
						int curDistance = (x - sx) * (x - sx) + (y - sy) * (y - sy);
						if (curDistance > distance) continue;
						SampleRes sample = Sample(x, y);
						int val = (sample.r + sample.g + sample.b) == 765 ? 1 : 0;
						if (ref != val)
						{
							distance = curDistance;
						}
					}
				}

				if (ref)
				{
					distanceMap[dx][dy] = distance;				
					maxValue = std::max(maxValue, distance);
				}
				else
				{
					distanceMap[dx][dy] = -distance;
					minValue = std::min(minValue, -distance);
				}
			}
		}

		size_t destPitch = elementSize * output.uWidth;
		auto Write = [destImage, destPitch, elementSize](size_t x, size_t y, int val)
		{
			unsigned char cVal = (int)std::max(0, std::min(val, 255));
			unsigned char* data = (unsigned char*)((size_t)destImage + y * destPitch + x * elementSize);
			data[0] = data[1] = data[2] = cVal;
			data[3] = 255;
		};

		minValue = -(int)sqrt(-minValue);
		maxValue = (int)sqrt(maxValue);

		const int e = -1;

		for (int dx = 0; dx < (int)output.uWidth; ++dx)
		{
			for (int dy = 0; dy < (int)output.uHeight; ++dy)
			{
				distanceMap[dx][dy] = (distanceMap[dx][dy] > 0) ? (int)sqrt(distanceMap[dx][dy]) : -(int)sqrt(-distanceMap[dx][dy]);
				int val = (2 * distanceMap[dx][dy] - 2 * e + maxValue - minValue) * 255 / (2 * (maxValue - minValue));
				Write(dx, dy, val);
			}
		}

		for (size_t i = 0; i < output.uWidth; ++i)
		{
			KDELETE[] distanceMap[i];
		}
		KDELETE[] distanceMap;

		codec->Save(output, "C:/Users/Admin/Desktop/SDF/textures/sdf.png");
	}

	sys->UnInit();
	KFileSystem::DestroyFileManager();
	KCodec::DestroyCodecManager();
	KLog::DestroyLogger();
}