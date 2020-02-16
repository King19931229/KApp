#pragma once
#include <initializer_list>
#include <assert.h>
#include <memory>

template<typename T, size_t DIMENSION = 1>
class KEPropertyModel
{
protected:
	T* m_SelfValue = nullptr;
	T* m_Value = nullptr;

	typedef KEPropertyModel<T, DIMENSION> ModelType;

	inline void Construct(std::initializer_list<T> list) noexcept
	{
		assert(list.size() == DIMENSION);
		for (size_t i = 0; i < list.size(); ++i)
		{
			m_Value[i] = std::move(*(list.begin() + i));
		}
	}

	inline void Construct(const ModelType& rhs) noexcept
	{
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Value[i] = rhs.m_Value[i];
		}
	}

	inline void Construct(ModelType&& rhs) noexcept
	{
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Value[i] = std::move(rhs.m_Value[i]);
		}
	}

	inline void EmptyConstruct() noexcept
	{
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Value[i] = std::move(T());
		}
	}

	template<typename T2>
	inline bool Less(const T2& lhs, const T2& rhs) const
	{
		return lhs < rhs;
	}

	template<typename T2>
	inline bool Greater(const T2& lhs, const T2& rhs) const
	{
		return lhs > rhs;
	}

	template<typename T2>
	inline bool Compare(const T2& lhs, const T2& rhs) const
	{
		return lhs == rhs;
	}

	template<>
	inline bool Compare(const float& lhs, const float& rhs) const
	{
		constexpr float precision = 0.0001f;
		return std::abs(lhs - rhs) < precision;
	}

	template<>
	inline bool Compare(const double& lhs, const double& rhs) const
	{
		constexpr double precision = 0.0001;
		return std::abs(lhs - rhs) < precision;
	}
public:
	KEPropertyModel() noexcept
		: m_SelfValue(new T [DIMENSION]),
		m_Value(m_SelfValue)
	{
		EmptyConstruct();
	}

	KEPropertyModel(const T& value) noexcept
		: m_SelfValue(new T[DIMENSION]),
		m_Value(m_SelfValue)
	{
		assert(DIMENSION == 1 && "dimension larger than 1 is not supported");
		m_Value[0] = value;
	}

	KEPropertyModel(T&& value) noexcept
		: m_SelfValue(new T[DIMENSION]),
		m_Value(m_SelfValue)
	{
		assert(DIMENSION == 1 && "dimension larger than 1 is not supported");
		m_Value[0] = std::move(value);
	}

	KEPropertyModel(std::initializer_list<T> list) noexcept
		: m_SelfValue(new T[DIMENSION]),
		m_Value(m_SelfValue)
	{
		Construct(list);
	}

	KEPropertyModel(const KEPropertyModel& rhs) noexcept
		: m_SelfValue(new T[DIMENSION]),
		m_Value(m_SelfValue)
	{
		Construct(rhs);
	}

	KEPropertyModel(KEPropertyModel&& rhs) noexcept
		: m_SelfValue(std::move(rhs.m_SelfValue)),
		m_Value(m_SelfValue)
	{
		rhs.m_SelfValue = nullptr;
		Construct(rhs);
	}

	KEPropertyModel(T reference[DIMENSION]) noexcept
		: m_SelfValue(nullptr),
		m_Value(reference)
	{
	}

	~KEPropertyModel()
	{
		SAFE_DELETE_ARRAY(m_SelfValue);
	}

	constexpr size_t Diemension() const
	{
		return DIMENSION;
	}

	ModelType& operator=(const T& value) noexcept
	{
		static_assert(DIMENSION == 1, "dimension larger than 1 is not supported");
		m_Value[0] = value;
		return *this;
	}

	ModelType& operator=(T&& value) noexcept
	{
		static_assert(DIMENSION == 1, "dimension larger than 1 is not supported");
		m_Value[0] = std::move(value);
		return *this;
	}

	ModelType& operator=(const ModelType& rhs) noexcept
	{
		Construct(rhs);
		return *this;
	}

	ModelType& operator=(ModelType&& rhs) noexcept
	{
		Construct(std::move(rhs));
		return *this;
	}

	ModelType& operator=(std::initializer_list<T> list) noexcept
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
		static_assert(DIMENSION == 1, "dimension larger than 1 is not comparable");
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

	operator const T&() const
	{
		assert(DIMENSION == 1 && "dimension larger than 1 is not supported");
		return m_Value[0];
	}

	operator T&()
	{
		assert(DIMENSION == 1 && "dimension larger than 1 is not supported");
		return m_Value[0];
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

	template<typename T, size_t DIMENTSION>
	inline std::shared_ptr<KEPropertyModel<T, DIMENTSION>> MakePropertyModel(std::initializer_list<T>&& list)
	{
		return std::shared_ptr<KEPropertyModel<T, DIMENTSION>>(new KEPropertyModel<T, DIMENTSION>(std::forward<decltype(list)>(list)));
	}
}