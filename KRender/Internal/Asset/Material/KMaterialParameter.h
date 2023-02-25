#pragma once
#include "Interface/IKMaterial.h"
#include "KMaterialValue.h"

class KMaterialParameter : public IKMaterialParameter
{
protected:
	struct Key
	{
		std::string name;
		size_t hash;
		size_t index;

		Key(const std::string& inName, size_t inHash, size_t inIndex)
		{
			name = inName;
			hash = inHash;
			index = inIndex;
		}
	};

	typedef IKMaterialValuePtr Value;
	std::vector<Key> m_ParameterKey;
	std::vector<Value> m_ParameterValue;

	IKMaterialValuePtr MakeValue(const std::string& name, MaterialValueType type, uint8_t dimension);
public:
	KMaterialParameter();
	~KMaterialParameter();

	bool HasValue(const std::string& name) const override;
	IKMaterialValuePtr GetValue(const std::string& name) const override;
	const std::vector<IKMaterialValuePtr>& GetAllValues() const override;
	bool CreateValue(const std::string& name, MaterialValueType type, uint8_t dimension, const void* initData = nullptr) override;
	bool SetValue(const std::string& name, MaterialValueType type, uint8_t vecSize, const void* data) override;
	bool RemoveValue(const std::string& name) override;
	bool RemoveAllValues() override;

	bool Duplicate(IKMaterialParameterPtr& parameter) override;
	bool Paste(const IKMaterialParameterPtr& parameter) override;
};