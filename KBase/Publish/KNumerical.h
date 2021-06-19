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

	template<typename T>
	T Factor2LessEqual(T num)
	{
		if(num > 0)
		{
			return ((num - (T)1) >> (T)1) <<(T) 1;
		}
		else
		{
			assert(false && "num is not greater than 0");
			return 0;
		}
	}

	template<typename T>
	T Factor2GreaterEqual(T num)
	{
		if(num > 0)
		{
			return ((num + (T)1) >> (T)1) <<(T) 1;
		}
		else
		{
			assert(false && "num is not greater than 0");
			return 0;
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

	template<typename T>
	T GCD(T a, T b)
	{
		T c;
		if(a < b)
		{
			c = a;
			a = b;
			b = c;
		}
		while(b)
		{
			c = a % b;
			a = b;
			b = c;
		}
		return a;
	}

	template<typename T>
	T LCM(T a, T b)
	{
		return a * b / GCD(a, b);
	}

	template<typename T>
	T AlignedSize(T value, T alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	// 把模板参数当做返回值技巧是把返回值作为第一个模板参数
	template<typename T2, typename T1>
	T2 Morton(T1 row, T1 col)
	{
		T2 morton = 0;
		T2 a = 0, b = 0;
		for (int i = 0; i < sizeof(uint32_t) * 8; i++)
		{
			a = (row & (uint64_t)1 << i);
			b = (col & (uint64_t)1 << i);
			morton |= a << (i + 1) | b << (i);
		}
		/*
		T2 c = 1;
		for (int i = 0; i < sizeof(uint32_t) * 8; i++)
		{
			a |= (row >> i & 1) * c;
			b |= (col >> i & 1) * c;
			c <<= 2;
		}
		morton = a * 2 + b;
		*/
		return morton;
	}
}