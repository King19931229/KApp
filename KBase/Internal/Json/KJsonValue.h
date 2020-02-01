#pragma once
#include "Interface/IKJson.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

class KJsonValue : public IKJsonValue
{
	friend class KJsonDocument;
protected:
	rapidjson::Value m_SelfValue;
	rapidjson::Value& m_Value;
	rapidjson::Document::AllocatorType& m_Allocator;

	void SetArray();
	void SetBool(bool value);
	void SetInt(int value);
	void SetFloat(float value);
	void SetString(const char* value);
	void SetObject();
public:
	KJsonValue(rapidjson::Document::AllocatorType& alloc);
	KJsonValue(rapidjson::Value& value, rapidjson::Document::AllocatorType& alloc);
	virtual ~KJsonValue();

	virtual size_t Size();
	virtual IKJsonValuePtr GetArrayElement(size_t index);

	virtual bool IsArray();
	virtual bool IsBool();
	virtual bool IsInt();
	virtual bool IsFloat();
	virtual bool IsString();
	virtual bool IsObject();

	virtual bool GetBool();
	virtual int GetInt();
	virtual float GetFloat();
	virtual std::string GetString();

	virtual bool Push(IKJsonValuePtr value);

	virtual bool HasMember(const char* key);
	virtual IKJsonValuePtr GetMember(const char* key);
	virtual bool AddMember(const char* key, IKJsonValuePtr value);
};