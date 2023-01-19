#ifndef _KERNAL_H_
#define _KERNAL_H_

#if defined(BOX_KERNEL_3X3)
	const uint Radius = 1;
	const uint Width = 1 + 2 * Radius;
	const float Kernel1D[Width] = { 1. / 3, 1. / 3, 1. / 3 };
	const float Kernel[Width][Width] =
	{
		{ 1. / 9, 1. / 9, 1. / 9 },
		{ 1. / 9, 1. / 9, 1. / 9 },
		{ 1. / 9, 1. / 9, 1. / 9 },
	};

#elif defined(BOX_KERNEL_5X5)
	const uint Radius = 2;
	const uint Width = 1 + 2 * Radius;
	const float Kernel1D[Width] = { 1. / 5, 1. / 5, 1. / 5, 1. / 5, 1. / 5 };
	const float Kernel[Width][Width] =
	{
		{ 1. / 25, 1. / 25, 1. / 25, 1. / 25, 1. / 25  },
		{ 1. / 25, 1. / 25, 1. / 25, 1. / 25, 1. / 25  },
		{ 1. / 25, 1. / 25, 1. / 25, 1. / 25, 1. / 25  },
		{ 1. / 25, 1. / 25, 1. / 25, 1. / 25, 1. / 25  },
		{ 1. / 25, 1. / 25, 1. / 25, 1. / 25, 1. / 25  },
	};

#elif defined(BOX_KERNEL_7X7)
	const uint Radius = 3;
	const uint Width = 1 + 2 * Radius;
	const float Kernel1D[Width] = { 1. / 7, 1. / 7, 1. / 7, 1. / 7, 1. / 7, 1. / 7, 1. / 7 };
	const float Kernel[Width][Width] =
	{
		{ 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49  },
		{ 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49  },
		{ 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49  },
		{ 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49  },
		{ 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49  },
		{ 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49  },
		{ 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49, 1. / 49  },
	};

#elif defined(GAUSSIAN_KERNEL_3X3)
	const uint Radius = 1;
	const uint Width = 1 + 2 * Radius;
	const float Kernel1D[Width] = { 0.27901, 0.44198, 0.27901 };
	const float Kernel[Width][Width] =
	{
		{ Kernel1D[0] * Kernel1D[0], Kernel1D[0] * Kernel1D[1], Kernel1D[0] * Kernel1D[2] },
		{ Kernel1D[1] * Kernel1D[0], Kernel1D[1] * Kernel1D[1], Kernel1D[1] * Kernel1D[2] },
		{ Kernel1D[2] * Kernel1D[0], Kernel1D[2] * Kernel1D[1], Kernel1D[2] * Kernel1D[2] },
	};

#elif defined(GAUSSIAN_KERNEL_5X5)
	const uint Radius = 2;
	const uint Width = 1 + 2 * Radius;
	const float Kernel1D[Width] = { 1. / 16, 1. / 4, 3. / 8, 1. / 4, 1. / 16 };
	const float Kernel[Width][Width] =
	{
		{ Kernel1D[0] * Kernel1D[0], Kernel1D[0] * Kernel1D[1], Kernel1D[0] * Kernel1D[2], Kernel1D[0] * Kernel1D[3], Kernel1D[0] * Kernel1D[4] },
		{ Kernel1D[1] * Kernel1D[0], Kernel1D[1] * Kernel1D[1], Kernel1D[1] * Kernel1D[2], Kernel1D[1] * Kernel1D[3], Kernel1D[1] * Kernel1D[4] },
		{ Kernel1D[2] * Kernel1D[0], Kernel1D[2] * Kernel1D[1], Kernel1D[2] * Kernel1D[2], Kernel1D[2] * Kernel1D[3], Kernel1D[2] * Kernel1D[4] },
		{ Kernel1D[3] * Kernel1D[0], Kernel1D[3] * Kernel1D[1], Kernel1D[3] * Kernel1D[2], Kernel1D[3] * Kernel1D[3], Kernel1D[3] * Kernel1D[4] },
		{ Kernel1D[4] * Kernel1D[0], Kernel1D[4] * Kernel1D[1], Kernel1D[4] * Kernel1D[2], Kernel1D[4] * Kernel1D[3], Kernel1D[4] * Kernel1D[4] },
	};

#elif defined(GAUSSIAN_KERNEL_7X7)
	const uint Radius = 3;
	const uint Width = 1 + 2 * Radius;
	const float Kernel1D[Width] = { 0.00598, 0.060626, 0.241843, 0.383103, 0.241843, 0.060626, 0.00598 };

#elif defined(GAUSSIAN_KERNEL_9X9)
	const uint Radius = 4;
	const uint Width = 1 + 2 * Radius;
	const float Kernel1D[Width] = { 0.000229, 0.005977, 0.060598, 0.241732, 0.382928, 0.241732, 0.060598, 0.005977, 0.000229 };
#endif

#endif