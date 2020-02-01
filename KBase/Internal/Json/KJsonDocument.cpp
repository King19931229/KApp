#include "KJsonDocument.h"
#include "KJsonValue.h"
#include <fstream>
#include <strstream>

EXPORT_DLL IKJsonDocumentPtr GetJsonDocument()
{
	return IKJsonDocumentPtr(new KJsonDocument());
}

KJsonDocument::KJsonDocument()
{
	m_Doc.SetObject();
}

KJsonDocument::~KJsonDocument()
{
}

IKJsonValuePtr KJsonDocument::GetRoot()
{
	return IKJsonValuePtr(new KJsonValue(m_Doc, m_Doc.GetAllocator()));
}

bool KJsonDocument::ParseFromFile(const char* jsonFile)
{
	std::string str;

	std::ifstream infile(jsonFile, std::ios::trunc);

	infile.open(jsonFile, std::ifstream::in);
	infile.seekg(0, std::ios::end);
	str.reserve(infile.tellg());
	infile.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
	infile.close();

	return ParseFromString(str.c_str());
}

bool KJsonDocument::ParseFromString(const char* jsonStr)
{
	return m_Doc.Parse(jsonStr).HasParseError();
}

bool KJsonDocument::SaveAsFile(const char* jsonFile)
{
	rapidjson::StringBuffer strBuffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(strBuffer);

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
	IKJsonValuePtr ret = IKJsonValuePtr(new KJsonValue(m_Doc[key], m_Doc.GetAllocator()));
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateObject()
{
	KJsonValue* jsonValue = new KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetObject();
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateArray()
{
	KJsonValue* jsonValue = new KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetArray();
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateBool(bool value)
{
	KJsonValue* jsonValue = new KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetBool(value);
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateInt(int value)
{
	KJsonValue* jsonValue = new KJsonValue(m_Doc.GetAllocator());
	jsonValue->SetInt(value);
	IKJsonValuePtr ret = IKJsonValuePtr(jsonValue);
	return ret;
}

IKJsonValuePtr KJsonDocument::CreateFloat(float value)
{
	KJsonValue* jsonValue = new KJsonValue(m_Doc.GetAllocator());
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