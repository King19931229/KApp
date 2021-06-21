#define MEMORY_DUMP_DEBUG
#include "KBase/Interface/IKReflection.h"
#include "glm/glm.hpp"
#include <iostream>

struct ParamterClass
{
	int x = 0;
};

struct TestBaseClass
{
	RTTR_ENABLE()
	RTTR_REGISTRATION_FRIEND
};

struct TestBaseClass2
{
	RTTR_ENABLE()
	RTTR_REGISTRATION_FRIEND
};

struct TestClass : public TestBaseClass, TestBaseClass2
{
	TestClass() {}

	void perform_calculation()
	{
		value += 12;
	}

	void perform_calculation(int new_value)
	{
		value += new_value;
	}

	void test_func(const ParamterClass& c)
	{
	}

	glm::vec3 member;
	int value = 0;

	void SetValue(int v)
	{
		value = v;
	}
	int GetValue() const
	{
		return value;
	}

	RTTR_ENABLE(TestBaseClass, TestBaseClass2)
	RTTR_REGISTRATION_FRIEND
};

RTTR_REGISTRATION
{
#define KRTTR_REG_CLASS_NAME TestClass
#define KRTTR_REG_CLASS_NAME_STR "TestClass"

	KRTTR_REG_CLASS_BEGIN()
	KRTTR_REG_PROPERTY_GET_SET(value, GetValue, SetValue, MDT_INT)
	KRTTR_REG_PROPERTY(member, MDT_FLOAT3)
	KRTTR_REG_METHOD(void(void), perform_calculation)
	KRTTR_REG_METHOD(void(int), perform_calculation)
	KRTTR_REG_METHOD(void(const ParamterClass&), test_func)
	KRTTR_REG_CLASS_END()

#undef KRTTR_REG_CLASS_NAME_STR
#undef KRTTR_REG_CLASS_NAME
}

int main()
{
	auto t = rttr::type::get_by_name("TestClass");
	for (auto meth : t.get_methods())
	{
		std::cout << meth.get_signature() << std::endl;
	}

	/*
	for (auto& prop : t.get_properties())
	{
		std::cout << "name: " << prop.get_name() << std::endl;
		auto type = prop.get_type();
		std::cout << "type name: " << type.get_name() << std::endl;
		auto metadata = prop.get_metadata("id");
		if (metadata.is_valid())
		{
			std::cout << "metadata " << metadata.get_value<int>() << std::endl;
		}
		metadata = prop.get_metadata("id2");
		if (metadata.is_valid())
		{
			std::cout << "metadata " << metadata.get_value<int>() << std::endl;
		}
	}
	*/

	TestBaseClass* obj = KNEW TestClass();

	auto type = rttr::type::get(*obj);

	auto method = type.get_method("perform_calculation");
	if (method.is_valid())
	{
		method.invoke(*obj, 20);
	}

	auto value = type.get_property("value");
	if (value.is_valid())
	{
		auto var_prop = value.get_value(obj);
		auto meta = value.get_metadata(META_DATA_TYPE);
		if (meta.is_valid())
		{
			MetaDataType metaDataType = meta.get_value<MetaDataType>();
			std::cout << "metaDataType " << metaDataType << std::endl;
		}
	}

	std::cout << type.get_name() << std::endl;

	delete obj;
}