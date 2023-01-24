#define MEMORY_DUMP_DEBUG
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKLog.h"
#include <array>
#include <tuple>

void SDF()
{
	IKCodecPtr codec = KCodec::GetCodec("C:/Users/Admin/Desktop/source.png");
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

		codec->Save(output, "C:/Users/Admin/Desktop/sdf.png");
	}
}

int main()
{
	KLog::CreateLogger();
	KCodec::CreateCodecManager();
	KFileSystem::CreateFileManager();

	IKFileSystemPtr sys = KFileSystem::CreateFileSystem(FST_NATIVE);
	sys->Init();
	KFileSystem::Manager->SetFileSystem(FSD_RESOURCE, sys);

	const int MAX_COMPONENT_COUNT = 5;

	const int componentCount = 5;
	const int radius = 8;

	std::vector<std::array<float, 4>> kernalParams[MAX_COMPONENT_COUNT];

	kernalParams[0].push_back({ 1.624835f, -0.862325f, 0.767583f, 1.862321f });

	kernalParams[1].push_back({ 5.268909f, -0.886528f,  0.411259f, -0.548794f });
	kernalParams[1].push_back({ 1.558213f, -1.960518f, 0.513282f, 4.561110f });

	kernalParams[2].push_back({ 5.043495f, -2.176490f, 1.621035f, -2.105439f });
	kernalParams[2].push_back({ 9.027613f, -1.019306f, -0.280860f, -0.162882f });
	kernalParams[2].push_back({ 1.597273f, -2.815110f, -0.366471f, 10.300301f });

	kernalParams[3].push_back({ 1.553635f, -4.338459f, -5.767909f, 46.164397f });
	kernalParams[3].push_back({ 4.693183f, -3.839993f, 9.795391f, 15.227561f });
	kernalParams[3].push_back({ 8.178137f, -2.791880f, -3.048324f, 0.302959f });
	kernalParams[3].push_back({ 12.328289f, -1.342190f, 0.010001f, 0.244650f });

	kernalParams[4].push_back({ 1.685979f, -4.892608f, -22.356787f, 85.912460f });
	kernalParams[4].push_back({ 4.998496f, -4.711870f, 35.918936f , -28.875618f });
	kernalParams[4].push_back({ 8.244168f, -4.052795f, -13.212253f, -1.578428f });
	kernalParams[4].push_back({ 11.900859f, -2.929212f,  0.507991f, 1.816328f });
	kernalParams[4].push_back({ 16.116382f, -1.512961f, 0.138051f, -0.010000f });

	auto ComputeKernal = [](float weightMagnitude, float weightPhase, float weightReal, float weightImaginary, float x)->std::array<float, 4>
	{
		return std::array<float, 4>
		{
			cosf(x* x* weightMagnitude)* expf(x* x* weightPhase),
				sinf(x* x* weightMagnitude)* expf(x* x* weightPhase),
				weightReal,
				weightImaginary
		};
	};

	std::vector<std::array<float, 4>> kernals[MAX_COMPONENT_COUNT];
	std::array<float, 2> weights[MAX_COMPONENT_COUNT];

	int paramsIndex = componentCount - 1;
	for (int component = 0; component < componentCount; ++component)
	{
		for (int i = 0; i < 2 * radius + 1; ++i)
		{
			kernals[component].push_back(ComputeKernal(
				kernalParams[paramsIndex][component][0],
				kernalParams[paramsIndex][component][1],
				kernalParams[paramsIndex][component][2],
				kernalParams[paramsIndex][component][3],
				float(i - radius) / float(radius)));
		}
	}

	for (int component = 0; component < componentCount; ++component)
	{
		weights[component] = { kernalParams[paramsIndex][component][2], kernalParams[paramsIndex][component][3] };
	}

	float sum = 0;
	for (int component = 0; component < componentCount; ++component)
	{
		for (int y = 0; y < 2 * radius + 1; ++y)
		{
			for (int x = 0; x < 2 * radius + 1; ++x)
			{
				std::array<float, 4> kx = kernals[component][x];
				std::array<float, 4> ky = kernals[component][y];
				sum += kx[2] * (kx[0] * ky[0] - kx[1] * ky[1]) + kx[3] * (kx[1] * ky[0] + kx[0] * ky[1]);
			}
		}
	}
	float normalize = 1.0f / sqrtf(sum);

	std::vector<std::array<float, 4>> normalizeKernals[MAX_COMPONENT_COUNT];

	for (int component = 0; component < componentCount; ++component)
	{
		for (int i = 0; i < 2 * radius + 1; ++i)
		{
			std::array<float, 4> k = kernals[component][i];
			normalizeKernals[component].push_back({ k[0] * normalize, k[1] * normalize, 0, 0 });
		}
	}

	std::array<float, 2> kernalMin[MAX_COMPONENT_COUNT];
	for (int component = 0; component < componentCount; ++component)
	{
		kernalMin[component] = { FLT_MAX, FLT_MAX };
		for (int i = 0; i < 2 * radius + 1; ++i)
		{
			const std::array<float, 4>& k = normalizeKernals[component][i];
			kernalMin[component][0] = std::min(kernalMin[component][0], k[0]);
			kernalMin[component][1] = std::min(kernalMin[component][1], k[1]);
		}
	}

	std::array<float, 2> kernalScale[MAX_COMPONENT_COUNT];
	for (int component = 0; component < componentCount; ++component)
	{
		kernalScale[component] = { 0, 0 };
		for (int i = 0; i < 2 * radius + 1; ++i)
		{
			const std::array<float, 4>& k = normalizeKernals[component][i];
			kernalScale[component][0] += k[0] - kernalMin[component][0];
			kernalScale[component][1] += k[1] - kernalMin[component][1];
		}
	}

	for (int component = 0; component < componentCount; ++component)
	{
		for (int i = 0; i < 2 * radius + 1; ++i)
		{
			std::array<float, 4>& k = normalizeKernals[component][i];
			k[2] = (k[0] - kernalMin[component][0]) / kernalScale[component][0];
			k[3] = (k[1] - kernalMin[component][1]) / kernalScale[component][1];
		}
	}

	{
		KCodecResult input;
		IKCodecPtr codec = KCodec::GetCodec("C:/Users/Admin/Desktop/example.jpg");
		if (codec->Codec("C:/Users/Admin/Desktop/example.jpg", true, input))
		{
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

			KCodecResult output = input;

			size_t destPitch = elementSize * output.uWidth;
			size_t destImageSize = 0;
			KImageHelper::GetByteSize(output.eFormat, output.uWidth, output.uHeight, output.uDepth, destImageSize);

			output.pData = KImageDataPtr(KNEW KImageData(destImageSize));
			output.pData->GetSubImageInfo().push_back(KSubImageInfo());
			unsigned char* destImage = output.pData->GetData();

			auto Write = [destImage, destPitch, elementSize](size_t x, size_t y, SampleRes result)
			{
				unsigned char* data = (unsigned char*)((size_t)destImage + y * destPitch + x * elementSize);
				data[0] = result.r;
				data[1] = result.g;
				data[2] = result.b;
				data[3] = 255;
			};

#if 0
			for (int y = 0; y < input.uHeight; ++y)
			{
				for (int x = 0; x < input.uWidth; ++x)
				{
					float sum = 0;
					float convolveR = 0;
					float convolveG = 0;
					float convolveB = 0;
					float convolveA = 0;
					for (int sy = 0; sy < 2 * radius + 1; ++sy)
					{
						for (int sx = 0; sx < 2 * radius + 1; ++sx)
						{
							int sampleX = x + sx - radius;
							int sampleY = y + sy - radius;

							sampleX = sampleX < 0 ? x : sampleX;
							sampleY = sampleY < 0 ? y : sampleY;

							sampleX = sampleX >= input.uWidth ? x : sampleX;
							sampleY = sampleY >= input.uHeight ? y : sampleY;

							SampleRes result = Sample(sampleX, sampleY);

							for (int c = 0; c < componentCount; ++c)
							{
								std::array<float, 4> kx = normalizeKernals[c][sx];
								std::array<float, 4> ky = normalizeKernals[c][sy];

								float real = kx[0] * ky[0] - kx[1] * ky[1];
								float imaginary = kx[0] * ky[1] + kx[1] * ky[0];

								float weightReal = weights[c][0];
								float weightImaginary = weights[c][1];

								sum += weightReal * real + weightImaginary * imaginary;

								convolveR += (weightReal * real * (float)result.r);
								convolveG += (weightReal * real * (float)result.g);
								convolveB += (weightReal * real * (float)result.b);
								convolveA += (weightReal * real * (float)result.a);

								convolveR += (weightImaginary * imaginary * (float)result.r);
								convolveG += (weightImaginary * imaginary * (float)result.g);
								convolveB += (weightImaginary * imaginary * (float)result.b);
								convolveA += (weightImaginary * imaginary * (float)result.a);
							}
						}
					}
					SampleRes convolve;
					convolve.r = std::max((unsigned char)0, std::min((unsigned char)255, (unsigned char)roundf(convolveR)));
					convolve.g = std::max((unsigned char)0, std::min((unsigned char)255, (unsigned char)roundf(convolveG)));
					convolve.b = std::max((unsigned char)0, std::min((unsigned char)255, (unsigned char)roundf(convolveB)));
					convolve.a = std::max((unsigned char)0, std::min((unsigned char)255, (unsigned char)roundf(convolveA)));
					Write(x, y, convolve);
				}
			}
#else
			std::vector<std::vector<std::tuple<float, float, float, float>>> middleRealResult;
			std::vector<std::vector<std::tuple<float, float, float, float>>> middleImaginaryResult;
			std::vector<std::vector<float>> middleRealSum;
			std::vector<std::vector<float>> middleImaginarySum;

			middleRealResult.resize(componentCount);
			middleImaginaryResult.resize(componentCount);
			middleRealSum.resize(componentCount);
			middleImaginarySum.resize(componentCount);

			for (int i = 0; i < componentCount; ++i)
			{
				middleRealResult[i].resize(output.uWidth * output.uHeight);
				middleImaginaryResult[i].resize(output.uWidth * output.uHeight);
				middleRealSum[i].resize(output.uWidth * output.uHeight);
				middleImaginarySum[i].resize(output.uWidth * output.uHeight);
			}

			auto SampleMiddleReal = [width = output.uWidth, &middleRealResult](size_t x, size_t y, int c)
			{
				size_t index = y * width + x;
				return middleRealResult[c][index];
			};
			auto SampleMiddleImaginary = [width = output.uWidth, &middleImaginaryResult](size_t x, size_t y, int c)
			{
				size_t index = y * width + x;
				return middleImaginaryResult[c][index];
			};
			auto WriteMiddleReal = [width = output.uWidth, &middleRealResult](size_t x, size_t y, int c, float r, float g, float b, float a)
			{
				size_t index = y * width + x;
				middleRealResult[c][index] = std::make_tuple(r, g, b, a);
			};
			auto WriteMiddleImaginary = [width = output.uWidth, &middleImaginaryResult](size_t x, size_t y, int c, float r, float g, float b, float a)
			{
				size_t index = y * width + x;
				middleImaginaryResult[c][index] = std::make_tuple(r, g, b, a);
			};

			auto SampleMiddleSumReal = [width = output.uWidth, &middleRealSum](size_t x, size_t y, int c)
			{
				size_t index = y * width + x;
				return middleRealSum[c][index];
			};
			auto SampleMiddleSumImaginary = [width = output.uWidth, &middleImaginarySum](size_t x, size_t y, int c)
			{
				size_t index = y * width + x;
				return middleImaginarySum[c][index];
			};
			auto WriteMiddleSumReal = [width = output.uWidth, &middleRealSum](size_t x, size_t y, int c, float sum)
			{
				size_t index = y * width + x;
				middleRealSum[c][index] = sum;
			};
			auto WriteMiddleSumImaginary = [width = output.uWidth, &middleImaginarySum](size_t x, size_t y, int c, float sum)
			{
				size_t index = y * width + x;
				middleImaginarySum[c][index] = sum;
			};

			for (int c = 0; c < componentCount; ++c)
			{
				for (int y = 0; y < input.uHeight; ++y)
				{
					for (int x = 0; x < input.uWidth; ++x)
					{
						float convolveRealR = 0;
						float convolveRealG = 0;
						float convolveRealB = 0;
						float convolveRealA = 0;

						float convolveImaginaryR = 0;
						float convolveImaginaryG = 0;
						float convolveImaginaryB = 0;
						float convolveImaginaryA = 0;

						float sumReal = 0;
						float sumImaginary = 0;
						for (int sx = 0; sx < 2 * radius + 1; ++sx)
						{
							int sampleX = x + sx - radius;
							sampleX = sampleX < 0 ? x : sampleX;
							sampleX = sampleX >= input.uWidth ? x : sampleX;

							SampleRes result = Sample(sampleX, y);

							std::array<float, 4> ky = normalizeKernals[c][sx];

							float real = ky[0];
							float imaginary = ky[1];

							convolveRealR += (real * (float)result.r);
							convolveRealG += (real * (float)result.g);
							convolveRealB += (real * (float)result.b);
							convolveRealA += (real * (float)result.a);

							convolveImaginaryR += (imaginary * (float)result.r);
							convolveImaginaryG += (imaginary * (float)result.g);
							convolveImaginaryB += (imaginary * (float)result.b);
							convolveImaginaryA += (imaginary * (float)result.a);

							sumReal += real;
							sumImaginary += imaginary;
						}

						WriteMiddleReal(x, y, c, convolveRealR, convolveRealG, convolveRealB, convolveRealA);
						WriteMiddleImaginary(x, y, c, convolveImaginaryR, convolveImaginaryG, convolveImaginaryB, convolveImaginaryA);

						WriteMiddleSumReal(x, y, c, sumReal);
						WriteMiddleSumImaginary(x, y, c, sumImaginary);
					}
				}
			}

			for (int y = 0; y < input.uHeight; ++y)
			{
				for (int x = 0; x < input.uWidth; ++x)
				{
					float sum = 0;
					float convolveR = 0;
					float convolveG = 0;
					float convolveB = 0;
					float convolveA = 0;

					for (int sy = 0; sy < 2 * radius + 1; ++sy)
					{
						for (int c = 0; c < componentCount; ++c)
						{
							int sampleY = y + sy - radius;
							sampleY = sampleY < 0 ? y : sampleY;
							sampleY = sampleY >= input.uHeight ? y : sampleY;

							std::tuple<float, float, float, float> convolveReal = SampleMiddleReal(x, sampleY, c);
							std::tuple<float, float, float, float> convolveImaginary = SampleMiddleImaginary(x, sampleY, c);

							float middleSumReal = SampleMiddleSumReal(x, sampleY, c);
							float middleSumImaginary = SampleMiddleSumImaginary(x, sampleY, c);

							std::array<float, 4> ky = normalizeKernals[c][sy];

							float real = ky[0];
							float imaginary = ky[1];
							float weightReal = weights[c][0];
							float weightImaginary = weights[c][1];

							sum += (real * middleSumReal - imaginary * middleSumImaginary) * weightReal;
							sum += (real * middleSumImaginary + imaginary * middleSumReal) * weightImaginary;

							convolveR += (real * std::get<0>(convolveReal) - imaginary * std::get<0>(convolveImaginary)) * weightReal;
							convolveG += (real * std::get<1>(convolveReal) - imaginary * std::get<1>(convolveImaginary)) * weightReal;
							convolveB += (real * std::get<2>(convolveReal) - imaginary * std::get<2>(convolveImaginary)) * weightReal;
							convolveA += (real * std::get<3>(convolveReal) - imaginary * std::get<3>(convolveImaginary)) * weightReal;

							convolveR += (real * std::get<0>(convolveImaginary) + imaginary * std::get<0>(convolveReal)) * weightImaginary;
							convolveG += (real * std::get<1>(convolveImaginary) + imaginary * std::get<1>(convolveReal)) * weightImaginary;
							convolveB += (real * std::get<2>(convolveImaginary) + imaginary * std::get<2>(convolveReal)) * weightImaginary;
							convolveA += (real * std::get<3>(convolveImaginary) + imaginary * std::get<3>(convolveReal)) * weightImaginary;
						}
					}
					SampleRes convolve;
					convolve.r = std::max((unsigned char)0, std::min((unsigned char)255, (unsigned char)roundf(convolveR)));
					convolve.g = std::max((unsigned char)0, std::min((unsigned char)255, (unsigned char)roundf(convolveG)));
					convolve.b = std::max((unsigned char)0, std::min((unsigned char)255, (unsigned char)roundf(convolveB)));
					convolve.a = std::max((unsigned char)0, std::min((unsigned char)255, (unsigned char)roundf(convolveA)));
					Write(x, y, convolve);
				}
			}
#endif
			codec->Save(output, "C:/Users/Admin/Desktop/example_1.png");
		}
	}

	// display
	{
		sum = 0;
		std::vector<std::vector<float>> displays;
		float maxS = 0.0f;
		for (int y = 0; y < 2 * radius + 1; ++y)
		{
			std::vector<float> display;
			for (int x = 0; x < 2 * radius + 1; ++x)
			{
				float s = 0;
				for (int component = 0; component < componentCount; ++component)
				{
					std::array<float, 4> kx = normalizeKernals[component][x];
					std::array<float, 4> ky = normalizeKernals[component][y];

					float real = kx[0] * ky[0] - kx[1] * ky[1];
					float imaginary = kx[0] * ky[1] + kx[1] * ky[0];

					float weightReal = weights[component][0];
					float weightImaginary = weights[component][1];

					sum += real * weightReal + imaginary * weightImaginary;
					s += real * weightReal + imaginary * weightImaginary;
				}
				display.push_back(s);
				maxS = std::max(maxS, s);
			}
			displays.push_back(display);
		}

		for (int y = 0; y < 2 * radius + 1; ++y)
		{
			for (int x = 0; x < 2 * radius + 1; ++x)
			{
				displays[y][x] /= maxS;
			}
		}

		IKCodecPtr codec = KCodec::GetCodec("*.png");
		if (codec)
		{
			KCodecResult output;
			output.eFormat = IF_R8G8B8A8;
			output.uWidth = 2 * radius + 1;
			output.uHeight = 2 * radius + 1;

			size_t destImageSize = 0;
			KImageHelper::GetByteSize(output.eFormat, output.uWidth, output.uHeight, output.uDepth, destImageSize);
			size_t elementSize = 0;
			KImageHelper::GetElementByteSize(output.eFormat, elementSize);

			size_t destPitch = elementSize * output.uWidth;

			output.pData = KImageDataPtr(KNEW KImageData(destImageSize));
			output.pData->GetSubImageInfo().push_back(KSubImageInfo());

			unsigned char* destImage = output.pData->GetData();

			auto Write = [destImage, destPitch, elementSize](size_t x, size_t y, int val)
			{
				unsigned char cVal = (int)std::max(0, std::min(val, 255));
				unsigned char* data = (unsigned char*)((size_t)destImage + y * destPitch + x * elementSize);
				data[0] = data[1] = data[2] = cVal;
				data[3] = 255;
			};

			for (int y = 0; y < 2 * radius + 1; ++y)
			{
				for (int x = 0; x < 2 * radius + 1; ++x)
				{
					Write(x, y, int(roundf(displays[y][x] * 255.0f)));
				}
			}
			codec->Save(output, "C:/Users/Admin/Desktop/result.png");
		}
	}

	sys->UnInit();
	KFileSystem::DestroyFileManager();
	KCodec::DestroyCodecManager();
	KLog::DestroyLogger();
}