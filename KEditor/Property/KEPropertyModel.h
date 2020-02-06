#pragma once
#include <initializer_list>
#include <assert.h>

template<typename T, size_t DIMENTSION = 1>
class KEPropertyModel
{
protected:
	T m_Value[DIMENTSION];
	typedef KEPropertyModel<T, DIMENTSION> ModelType;

	void Construct(std::initializer_list<T> list)
	{
		assert(list.size() == DIMENTSION);
		for (size_t i = 0; i < list.size(); ++i)
		{
			m_Value[i] = *(list.begin() + i);
		}
	}

	void ZeroConstruct()
	{
		for (size_t i = 0; i < DIMENTSION; ++i)
		{
			m_Value[i] = T();
		}
	}

	template<typename T2>
	bool Less(const T2& lhs, const T2& rhs) const
	{
		return lhs < rhs;
	}

	template<typename T2>
	bool Greater(const T2& lhs, const T2& rhs) const
	{
		return lhs > rhs;
	}

	template<typename T2>
	bool Compare(const T2& lhs, const T2& rhs) const
	{
		return lhs == rhs;
	}

	template<>
	bool Compare(const float& lhs, const float& rhs) const
	{
		constexpr float precision = 0.0001f;
		return std::abs(lhs - rhs) < precision;
	}

	template<>
	bool Compare(const double& lhs, const double& rhs) const
	{
		constexpr double precision = 0.0001;
		return std::abs(lhs - rhs) < precision;
	}
public:
	KEPropertyModel()
	{
		ZeroConstruct();
	}

	KEPropertyModel(std::initializer_list<T> list)
	{
		Construct(list);
	}

	size_t Diemension() const
	{
		return DIMENTSION;
	}

	ModelType& operator=(std::initializer_list<T> list)
	{
		Construct(list);
		return *this;
	}

	T& operator[](size_t index)
	{
		assert(index < DIMENTSION);
		return m_Value[index];
	}

	bool operator==(const ModelType& rhs) const
	{
		for (size_t i = 0; i < DIMENTSION; ++i)
		{
			if (!Compare(m_Value[i], rhs.m_Value[i]))
			{
				return false;
			}
		}
		return true;
	}

	bool operator!=(const ModelType& rhs) const
	{
		return !operator==(rhs);
	}

	bool operator<=(const ModelType& rhs) const
	{
		static_assert(DIMENTSION == 1, "Is not comparable");
		return m_Value[0] <= rhs.m_Value[0];
	}

	bool operator>(const ModelType& rhs) const
	{
		return !operator<=(rhs);
	}

	bool operator>=(const ModelType& rhs) const
	{
		static_assert(DIMENTSION == 1, "Is not comparable");
		return m_Value[0] >= rhs.m_Value[0];
	}

	bool operator<(const ModelType& rhs) const
	{
		return !operator>=(rhs);
	}

	const T& operator[](size_t index) const
	{
		assert(index < DIMENTSION);
		return m_Value[index];
	}
};