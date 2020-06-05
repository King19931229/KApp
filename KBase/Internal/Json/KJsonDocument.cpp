#include "KJsonDocument.h"
#include "KJsonValue.h"
#include "rapidjson/prettywriter.h"
#include <fstream>
#include <strstream>

EXPORT_DLL IKJsonDocumentPtr GetJsonDocument()
{
	return IKJsonDocumentPtr(KNEW KJsonDocument());
}

KJsonDocument::KJsonDocument()
{
	m_Doc.SetObject();
	assert(m_Doc.IsObject());
}

KJsonDocument::~KJsonDocument()
{
}

IKJsonValuePtr KJsonDocument::GetRoot()
{
	return IKJsonValuePtr(KNEW KJsonValue(m_Doc, m_Doc.GetAllocator()));
}

bool KJsonDocument::ParseFromDataStream(IKDataStreamPtr dataStream)
{
	size_t size = dataStream->GetSize();

	std::vector<char> datas;
	datas.resize(size + 1);

	if (dataStream->Read(datas.data(), size))
	{
		datas[size] = '\0';
		return ParseFromString(datas.data());
	}

	return false;
}

bool KJsonDocument::ParseFromFile(const char* jsonFile)
{
	std::string str;

	std::ifstream infile(jsonFile, std::ios::trunc);

	infile.open(jsonFile, std::ifstream::in);
	infile.seekg(0, std::ios::end);
	str.reserve((size_t)infile.tellg());
	infile.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
	infile.close();

	return ParseFromString(str.c_str());
}

bool KJsonDocument::ParseFromString(const char* jsonStr)
{
	return !m_Doc.Parse(jsonStr).HasParseError();
}

bool KJsonDocument::SaveAsFile(const char* jsonFile)
{
	rapidjson::StringBuffer strBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(strBuffer);

	m_Doc.Accept(writer);

	std::ofstream outfile(jsonFile, std::ios::trunc);
	outfile << strBuffer.GetString() << std::endl;
	outfile.flush();
	outfile.close();

	return true;
}

bool KJsonDocument::HasMember(const char* key)
{
	return m_Doc.HasMember(key);
}

IKJsonValuePtr KJsonDocument::GetMember(const char* key)
{
	IKJsonValuePtr ret = IKJsonValuePtr(KNEW KJsonValue(m_Doc[key], m_Doc.GetAllocator()));
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateObject()
{
	KJsonValue* jsonValue = KNEW KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetObject();
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateArray()
{
	KJsonValue* jsonValue = KNEW KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetArray();
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateBool(bool value)
{
	KJsonValue* jsonValue = KNEW KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetBool(value);
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateInt(int value)
{
	KJsonValue* jsonValue = KNEW KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetInt(value);
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateFloat(float value)
{
	KJsonValue* jsonValue = KNEW KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetFloat(value);
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateString(const char* value)
{
	KJsonValue* jsonValue = new KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetString(value);
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}