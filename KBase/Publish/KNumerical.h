#include <assert.h>

namespace KNumerical
{
	template<typename T>
	bool IsPow2(T num)
	{
		if(num > 0)
		{
			for(num; (num & (T)1) == 0 ; num >>= (T)1){}
			return num == 1;			
		}
		else
		{
			assert(false && "num is not greater than 0");
			return 0;
		}
	}

	template<typename T>
	T Pow2LessEqual(T num)
	{
		if(num > 0)
		{
			T val = 0;
			for(T n = num; n > 0; n >>= (T)1)
				++val;
			val = (T)1 << (val - (T)1);
			return val;
		}
		else
		{
			assert(false && "num is not greater than 0");
			return 0;
		}
	}

	template<typename T>
	T Pow2GreaterEqual(T num)
	{
		if(num > 0)
		{
			T val = 0;
			for(T n = num; n > 0; n >>= (T)1)
				++val;
			val = (T)1 << (val - (T)1);
			if(val < num)
				val <<= (T)1;
			return val;
		}
		else
		{
			return 1;
		}
	}

	// 从0开始计算
	template<typename T>
	T HighestBinaryBit(T num)
	{
		if(num > 0)
		{
			T val = 0;
			for(T n = num; n > 0; n >>= (T)1)
				++val;
			return val - (T)1;
		}
		else
		{
			assert(false && "num is not greater than 0");
			return 0;
		}
	}

	template<typename T>
	T LowestBinaryBit(T num)
	{
		if(num > 0)
		{
			T val = 0;
			for(T n = num; (n & (T)1) == 0 ; n >>= (T)1)
				++val;
			return val;
		}
		else
		{
			assert(false && "num is not greater than 0");
			return 0;
		}
	}
}