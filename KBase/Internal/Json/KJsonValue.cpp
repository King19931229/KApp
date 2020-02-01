#include "KJsonValue.h"

KJsonValue::KJsonValue(rapidjson::Document::AllocatorType& alloc)
	: m_Value(m_SelfValue),
	m_Allocator(alloc)
{
}

KJsonValue::KJsonValue(rapidjson::Value& value, rapidjson::Document::AllocatorType& alloc)
	: m_Value(value),
	m_Allocator(alloc)
{
}

KJsonValue::~KJsonValue()
{
}

void KJsonValue::SetArray()
{
	m_Value.SetArray();
}

void KJsonValue::SetBool(bool value)
{
	m_Value.SetBool(value);
}

void KJsonValue::SetInt(int value)
{
	m_Value.SetInt(value);
}

void KJsonValue::SetFloat(float value)
{
	m_Value.SetFloat(value);
}

void KJsonValue::SetString(const char* value)
{
	m_Value.SetString(value, (rapidjson::SizeType)strlen(value), m_Allocator);
}

void KJsonValue::SetObject()
{
	m_Value.SetObject();
}

size_t KJsonValue::Size()
{
	return m_Value.Size();
}

IKJsonValuePtr KJsonValue::GetArrayElement(size_t index)
{
	return IKJsonValuePtr(new KJsonValue(m_Value[(rapidjson::SizeType)index], m_Allocator));
}

bool KJsonValue::IsArray()
{
	return m_Value.IsArray();
}

bool KJsonValue::IsBool()
{
	return m_Value.IsBool();
}

bool KJsonValue::IsInt()
{
	return m_Value.IsInt();
}

bool KJsonValue::IsFloat()
{
	return m_Value.IsFloat();
}

bool KJsonValue::IsString()
{
	return m_Value.IsString();
}

bool KJsonValue::IsObject()
{
	return m_Value.IsObject();
}

bool KJsonValue::GetBool()
{
	return m_Value.GetBool();
}

int KJsonValue::GetInt()
{
	return m_Value.GetInt();
}

float KJsonValue::GetFloat()
{
	return m_Value.GetFloat();
}

std::string KJsonValue::GetString()
{
	std::string ret = m_Value.GetString();
	assert(ret.length() == m_Value.GetStringLength());
	return std::move(ret);
}

bool KJsonValue::Push(IKJsonValuePtr value)
{
	m_Value.PushBack(((KJsonValue*)value.get())->m_Value, m_Allocator);
	return true;
}

bool KJsonValue::HasMember(const char* key)
{
	return m_Value.HasMember(key);
}

IKJsonValuePtr KJsonValue::GetMember(const char* key)
{
	return IKJsonValuePtr(new KJsonValue(m_Value[key], m_Allocator));
}

bool KJsonValue::AddMember(const char* key, IKJsonValuePtr value)
{
	rapidjson::Value keyJson = rapidjson::Value(key, (rapidjson::SizeType)strlen(key), m_Allocator);
	m_Value.AddMember(keyJson, ((KJsonValue*)value.get())->m_Value, m_Allocator);
	return true;
}