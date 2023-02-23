#include "KMaterialParameter.h"
#include "KBase/Publish/KHash.h"
#include <algorithm>
#include <assert.h>

KMaterialParameter::KMaterialParameter()
{
}

KMaterialParameter::~KMaterialParameter()
{
}

bool KMaterialParameter::HasValue(const std::string& name) const
{
	size_t hash = KHash::BKDR(name.c_str(), name.length());
	Key ref = { hash, 0 };
	auto it = std::lower_bound(m_ParameterKey.begin(), m_ParameterKey.end(), ref, [](const Key& lhs, const Key& rhs) { return lhs.hash < rhs.hash; });
	if (it != m_ParameterKey.end() && it->hash == hash)
	{
		return true;
	}
	return false;
}

IKMaterialValuePtr KMaterialParameter::GetValue(const std::string& name) const
{
	size_t hash = KHash::BKDR(name.c_str(), name.length());
	Key ref = { hash, 0 };
	auto it = std::lower_bound(m_ParameterKey.begin(), m_ParameterKey.end(), ref, [](const Key& lhs, const Key& rhs) { return lhs.hash < rhs.hash; });
	if (it != m_ParameterKey.end() && it->hash == hash)
	{
		return m_ParameterValue[it->index];
	}
	return nullptr;
}

const std::vector<IKMaterialValuePtr>& KMaterialParameter::GetAllValues() const
{
	return m_ParameterValue;
}

IKMaterialValuePtr KMaterialParameter::MakeValue(const std::string& name, MaterialValueType type, uint8_t dimension)
{
	switch (type)
	{
	case MaterialValueType::BOOL:
		if (dimension == 1)
		{
			return IKMaterialValuePtr(KNEW KMaterialBoolValue(name));
		}
		break;
	case MaterialValueType::INT:
		if (dimension == 1)
		{
			return IKMaterialValuePtr(KNEW KMaterialIntValue(name));
		}
		if (dimension == 2)
		{
			return IKMaterialValuePtr(KNEW KMaterialInt2Value(name));
		}
		if (dimension == 3)
		{
			return IKMaterialValuePtr(KNEW KMaterialInt3Value(name));
		}
		if (dimension == 4)
		{
			return IKMaterialValuePtr(KNEW KMaterialInt4Value(name));
		}
		break;
	case MaterialValueType::FLOAT:
		if (dimension == 1)
		{
			return IKMaterialValuePtr(KNEW KMaterialFloatValue(name));
		}
		if (dimension == 2)
		{
			return IKMaterialValuePtr(KNEW KMaterialFloat2Value(name));
		}
		if (dimension == 3)
		{
			return IKMaterialValuePtr(KNEW KMaterialFloat3Value(name));
		}
		if (dimension == 4)
		{
			return IKMaterialValuePtr(KNEW KMaterialFloat4Value(name));
		}
		break;
	default:
		break;
	}

	ASSERT_RESULT(false && "not support");
	return nullptr;
}

bool KMaterialParameter::CreateValue(const std::string& name, MaterialValueType type, uint8_t vecSize, const void* initData)
{
	size_t hash = KHash::BKDR(name.c_str(), name.length());
	Key ref = { hash, 0 };

	auto it = std::lower_bound(m_ParameterKey.begin(), m_ParameterKey.end(), ref, [](const Key& lhs, const Key& rhs) { return lhs.hash < rhs.hash; });
	if (it == m_ParameterKey.end() || it->hash != hash)
	{
		Value value = MakeValue(name, type, vecSize);

		for (auto itMod = it, itEnd = m_ParameterKey.end(); itMod != itEnd; ++itMod)
		{
			++itMod->index;
		}

		if (it == m_ParameterKey.end())
		{
			ref.index = m_ParameterKey.size();
			m_ParameterValue.push_back(value);
		}
		else
		{
			ref.index = it->index - 1;
			m_ParameterValue.insert(m_ParameterValue.begin() + ref.index, value);
		}
		m_ParameterKey.insert(it, ref);

		ASSERT_RESULT(m_ParameterKey.size() == m_ParameterValue.size());

		return true;
	}

	return false;
}

bool KMaterialParameter::SetValue(const std::string& name, MaterialValueType type, uint8_t vecSize, const void* data)
{
	size_t hash = KHash::BKDR(name.c_str(), name.length());
	Key ref = { hash, 0 };

	auto it = std::lower_bound(m_ParameterKey.begin(), m_ParameterKey.end(), ref, [](const Key& lhs, const Key& rhs) { return lhs.hash < rhs.hash; });
	if (it != m_ParameterKey.end() && it->hash == hash)
	{
		Value value = m_ParameterValue[it->index];
		value->SetData(data);
		return true;
	}

	return false;
}

bool KMaterialParameter::RemoveValue(const std::string& name)
{
	size_t hash = KHash::BKDR(name.c_str(), name.length());
	Key ref = { hash, 0 };

	auto it = std::lower_bound(m_ParameterKey.begin(), m_ParameterKey.end(), ref, [](const Key& lhs, const Key& rhs) { return lhs.hash < rhs.hash; });
	if (it != m_ParameterKey.end() && it->hash == hash)
	{
		for (auto itMod = it + 1, itEnd = m_ParameterKey.end(); itMod != itEnd; ++itMod)
		{
			--itMod->index;
		}

		m_ParameterValue.erase(m_ParameterValue.begin() + it->index);
		m_ParameterKey.erase(it);

		ASSERT_RESULT(m_ParameterKey.size() == m_ParameterValue.size());

		return true;
	}

	return false;
}

bool KMaterialParameter::RemoveAllValues()
{
	m_ParameterKey.clear();
	m_ParameterValue.clear();
	return true;
}

// TODO 逐 Key->Value 验证拷贝
bool KMaterialParameter::Duplicate(IKMaterialParameterPtr& parameter)
{
	parameter = IKMaterialParameterPtr(KNEW KMaterialParameter());
	KMaterialParameter* pParameter = (KMaterialParameter*)parameter.get();
	pParameter->m_ParameterKey = m_ParameterKey;
	pParameter->m_ParameterValue = m_ParameterValue;
	return true;
}

// TODO 逐 Key->Value 验证拷贝
bool KMaterialParameter::Paste(const IKMaterialParameterPtr& parameter)
{
	if (parameter)
	{
		m_ParameterKey = ((KMaterialParameter*)parameter.get())->m_ParameterKey;
		m_ParameterValue = ((KMaterialParameter*)parameter.get())->m_ParameterValue;
		return true;
	}
	return false;
}