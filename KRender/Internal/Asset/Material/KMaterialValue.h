#pragma once
#include "Interface/IKMaterial.h"

template<typename T, uint8_t VecSize>
class KMaterialValue : public IKMaterialValue
{
	static_assert(VecSize >= 1 && VecSize <= 4, "dimension out of bound");
protected:
	std::string m_Name;
	T m_Value[VecSize];

	template<typename T2>
	MaterialValueType GetValueType(T2) const
	{
		static_assert(false, "type not supported");
	}

	template<>
	MaterialValueType GetValueType<bool>(bool) const
	{
		return MaterialValueType::BOOL;
	}
	template<>
	MaterialValueType GetValueType<int>(int) const
	{
		return MaterialValueType::INT;
	}
	template<>
	MaterialValueType GetValueType<float>(float) const
	{
		return MaterialValueType::FLOAT;
	}

public:
	KMaterialValue(const std::string& name)
		: m_Name(name)
	{
		for (uint8_t i = 0; i < VecSize ; ++i)
		{
			m_Value[i] = static_cast<T>(0);
		}
	}
	~KMaterialValue() {}

	const std::string& GetName() const override
	{
		return m_Name;
	}

	MaterialValueType GetType() const override
	{
		return GetValueType(T());
	}

	uint8_t GetVecSize() const override
	{
		return VecSize;
	}

	const void* GetData() const override
	{
		return m_Value;
	}

	void SetData(const void* data) override
	{
		if (data)
		{
			memcpy(m_Value, data, sizeof(m_Value));
		}
		else
		{
			for (uint8_t i = 0; i < VecSize; ++i)
			{
				m_Value[i] = static_cast<T>(0);
			}
		}
	}
};

typedef KMaterialValue<bool, 1> KMaterialBoolValue;

typedef KMaterialValue<int, 1> KMaterialIntValue;
typedef KMaterialValue<int, 2> KMaterialInt2Value;
typedef KMaterialValue<int, 3> KMaterialInt3Value;
typedef KMaterialValue<int, 4> KMaterialInt4Value;

typedef KMaterialValue<float, 1> KMaterialFloatValue;
typedef KMaterialValue<float, 2> KMaterialFloat2Value;
typedef KMaterialValue<float, 3> KMaterialFloat3Value;
typedef KMaterialValue<float, 4> KMaterialFloat4Value;