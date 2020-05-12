#define MEMORY_DUMP_DEBUG
#include "KBase/Interface/IKReflection.h"
#include <iostream>

struct TestClass
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

	int value = 0;
};

int main()
{
	{
#define KRTTR_REG_CLASS_NAME TestClass
#define KRTTR_REG_CLASS_NAME_STR "TestClass"

		KRTTR_REG_CLASS_BEGIN()
			KRTTR_REG_PROPERTY(value)
			KRTTR_REG_METHOD(void(void), perform_calculation)
			KRTTR_REG_METHOD(void(int), perform_calculation)
		KRTTR_REG_CLASS_END()

#undef KRTTR_REG_CLASS_NAME_STR
#undef KRTTR_REG_CLASS_NAME
	}

	auto t = rttr::type::get_by_name("TestClass");
	for (auto meth : t.get_methods())
	{
		std::cout << meth.get_signature() << std::endl;
	}
}