#pragma once
#include "KBase/Publish/KConfig.h"
#include "KBase/Interface/IKDataStream.h"
#include <string>
#include <memory>
#include <vector>

struct IKJsonValue;
typedef std::shared_ptr<IKJsonValue> IKJsonValuePtr;

struct IKJsonValue
{
	virtual ~IKJsonValue() {}

	virtual size_t Size() = 0;
	virtual IKJsonValuePtr GetArrayElement(size_t index) = 0;

	virtual bool IsArray() = 0;
	virtual bool IsBool() = 0;
	virtual bool IsInt() = 0;
	virtual bool IsFloat() = 0;
	virtual bool IsString() = 0;
	virtual bool IsObject() = 0;

	virtual bool GetBool() = 0;
	virtual int GetInt() = 0;
	virtual float GetFloat() = 0;
	virtual std::string GetString() = 0;

	virtual bool Push(IKJsonValuePtr value) = 0;
	
	virtual bool HasMember(const char* key) = 0;
	virtual IKJsonValuePtr GetMember(const char* key) = 0;
	virtual bool AddMember(const char* key, IKJsonValuePtr value) = 0;	
};

struct IKJsonDocument;
typedef std::shared_ptr<IKJsonDocument> IKJsonDocumentPtr;

struct IKJsonDocument
{
	virtual ~IKJsonDocument() {}

	virtual IKJsonValuePtr GetRoot() = 0;

	virtual bool ParseFromDataStream(IKDataStreamPtr dataStream) = 0;
	virtual bool ParseFromFile(const char* jsonFile) = 0;
	virtual bool ParseFromString(const char* jsonStr) = 0;
	virtual bool SaveAsFile(const char* jsonFile) = 0;

	virtual bool HasMember(const char* key) = 0;
	virtual IKJsonValuePtr GetMember(const char* key) = 0;

	virtual IKJsonValuePtr CreateObject() = 0;
	virtual IKJsonValuePtr CreateArray() = 0;

	virtual IKJsonValuePtr CreateBool(bool value) = 0;
	virtual IKJsonValuePtr CreateInt(int value) = 0;
	virtual IKJsonValuePtr CreateFloat(float value) = 0;
	virtual IKJsonValuePtr CreateString(const char* value) = 0;
};

EXPORT_DLL IKJsonDocumentPtr GetJsonDocument();