#pragma once
#include "Interface/IKJson.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

class KJsonDocument : public IKJsonDocument
{
protected:
	rapidjson::Document m_Doc;
public:
	KJsonDocument();
	virtual ~KJsonDocument();

	virtual IKJsonValuePtr GetRoot();

	virtual bool ParseFromFile(const char* jsonFile);
	virtual bool ParseFromString(const char* jsonStr);
	virtual bool SaveAsFile(const char* jsonFile);

	virtual bool HasMember(const char* key);
	virtual IKJsonValuePtr GetMember(const char* key);

	virtual IKJsonValuePtr CreateObject();
	virtual IKJsonValuePtr CreateArray();

	virtual IKJsonValuePtr CreateBool(bool value);
	virtual IKJsonValuePtr CreateInt(int value);
	virtual IKJsonValuePtr CreateFloat(float value);
	virtual IKJsonValuePtr CreateString(const char* value);
};