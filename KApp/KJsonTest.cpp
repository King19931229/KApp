#define MEMORY_DUMP_DEBUG
#include "KBase/Interface/IKJson.h"

int main()
{
	{
		IKJsonDocumentPtr jsonDoc = GetJsonDocument();
		IKJsonValuePtr root = jsonDoc->GetRoot();

		root->AddMember("int", jsonDoc->CreateInt(10));
		root->AddMember("bool", jsonDoc->CreateBool(true));
		root->AddMember("float", jsonDoc->CreateFloat(20.0f));
		root->AddMember("key", jsonDoc->CreateString("value"));

		auto array = jsonDoc->CreateArray();
		array->Push(jsonDoc->CreateInt(10));
		array->Push(jsonDoc->CreateBool(true));
		array->Push(jsonDoc->CreateString("value"));

		root->AddMember("array", array);

		auto object = jsonDoc->CreateObject();
		object->AddMember("object_int", jsonDoc->CreateInt(10));
		object->AddMember("object_bool", jsonDoc->CreateBool(true));
		object->AddMember("object_float", jsonDoc->CreateFloat(20.0f));
		object->AddMember("object_key", jsonDoc->CreateString("value"));

		root->AddMember("object", object);

		jsonDoc->SaveAsFile("test.json");
	}

	{
		IKJsonDocumentPtr jsonDoc = GetJsonDocument();
		jsonDoc->ParseFromFile("test.json");
		IKJsonValuePtr root = jsonDoc->GetRoot();

		if (root->HasMember("int") && root->GetMember("int")->IsInt())
		{
			printf("%d\n", root->GetMember("int")->GetInt());
		}
		if (root->HasMember("bool") && root->GetMember("bool")->IsBool())
		{
			printf("%d\n", root->GetMember("bool")->GetBool());
		}
		if (root->HasMember("float") && root->GetMember("float")->IsFloat())
		{
			printf("%f\n", root->GetMember("float")->GetFloat());
		}
		if (root->HasMember("key") && root->GetMember("key")->IsString())
		{
			printf("%s\n", root->GetMember("key")->GetString().c_str());
		}

		if (root->HasMember("array") && root->GetMember("array")->IsArray())
		{
			auto array = root->GetMember("array");
			for (size_t i = 0; i < array->Size(); ++i)
			{
				auto element = array->GetArrayElement(i);
				if (element->IsInt())
				{
					printf("%d\n", element->GetInt());
				}
				if (element->IsBool())
				{
					printf("%d\n", element->GetBool());
				}
				if (element->IsFloat())
				{
					printf("%f\n", element->GetFloat());
				}
				if (element->IsString())
				{
					printf("%s\n", element->GetString().c_str());
				}
			}
		}

		if (root->HasMember("object") && root->GetMember("object")->IsObject())
		{
			auto object = root->GetMember("object");

			if (object->HasMember("object_int") && object->GetMember("object_int")->IsInt())
			{
				printf("%d\n", object->GetMember("object_int")->GetInt());
			}
			if (object->HasMember("object_bool") && object->GetMember("object_bool")->IsBool())
			{
				printf("%d\n", object->GetMember("object_bool")->GetBool());
			}
			if (object->HasMember("object_float") && object->GetMember("object_float")->IsFloat())
			{
				printf("%f\n", object->GetMember("object_float")->GetFloat());
			}
			if (object->HasMember("object_key") && object->GetMember("object_key")->IsString())
			{
				printf("%s\n", object->GetMember("object_key")->GetString().c_str());
			}
		}
	}
}