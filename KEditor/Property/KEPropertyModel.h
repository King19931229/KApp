#pragma once
#include <initializer_list>
#include <assert.h>
#include <memory>

template<typename T, size_t DIMENSION = 1>
class KEPropertyModel
{
protected:
	T m_SelfValue[DIMENSION];
	T* m_Value;
	typedef KEPropertyModel<T, DIMENSION> ModelType;

	void Construct(std::initializer_list<T> list)
	{
		assert(list.size() == DIMENSION);
		for (size_t i = 0; i < list.size(); ++i)
		{
			m_Value[i] = *(list.begin() + i);
		}
	}

	void Construct(const ModelType& rhs)
	{
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Value[i] = rhs.m_Value[i];
		}
	}

	void ZeroConstruct()
	{
		for (size_t i = 0; i < DIMENSION; ++i)
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
		: m_Value(m_SelfValue)
	{
		ZeroConstruct();
	}

	KEPropertyModel(T reference[DIMENSION])
		: m_Value(reference)
	{
	}

	KEPropertyModel(std::initializer_list<T> list)
		: m_Value(m_SelfValue)
	{
		Construct(list);
	}

	KEPropertyModel(KEPropertyModel& rhs)
		: m_Value(m_SelfValue)
	{
		Construct(rhs);
	}

	size_t Diemension() const
	{
		return DIMENSION;
	}

	ModelType& operator=(const ModelType& rhs)
	{
		Construct(rhs);
		return *this;
	}

	ModelType& operator=(std::initializer_list<T> list)
	{
		Construct(list);
		return *this;
	}

	bool operator==(const ModelType& rhs) const
	{
		for (size_t i = 0; i < DIMENSION; ++i)
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
		static_assert(DIMENSION == 1, "dimension lager than 1 is not comparable");
		return m_Value[0] <= rhs.m_Value[0];
	}

	bool operator>(const ModelType& rhs) const
	{
		return !operator<=(rhs);
	}

	bool operator>=(const ModelType& rhs) const
	{
		static_assert(DIMENSION == 1, "dimension lager than 1 is not comparable");
		return m_Value[0] >= rhs.m_Value[0];
	}

	bool operator<(const ModelType& rhs) const
	{
		return !operator>=(rhs);
	}

	const T& operator[](size_t index) const
	{
		assert(index < DIMENSION);
		return m_Value[index];
	}

	T& operator[](size_t index)
	{
		assert(index < DIMENSION);
		return m_Value[index];
	}
};

namespace KEditor
{
	template<typename T, size_t DIMENTSION = 1, typename... Types>
	inline std::shared_ptr<KEPropertyModel<T, DIMENTSION>> MakePropertyModel(Types&&... args)
	{
		return std::shared_ptr<KEPropertyModel<T, DIMENTSION>>
			(new KEPropertyModel<T, DIMENTSION>(std::forward<Types>(args)...));
	}
}