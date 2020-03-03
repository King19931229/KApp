#include "KXMLAttribute.h"
#include <assert.h>

KXMLAttribute::KXMLAttribute()
	: m_Attribute(nullptr),
	m_ReadOnlyAttribute(nullptr),
	m_ReadOnly(false)
{
}

KXMLAttribute::KXMLAttribute(tinyxml2::XMLAttribute* attribute)
	: m_Attribute(attribute),
	m_ReadOnlyAttribute(attribute),
	m_ReadOnly(false)
{
}

KXMLAttribute::KXMLAttribute(const tinyxml2::XMLAttribute* attribute)
	: m_Attribute(nullptr),
	m_ReadOnlyAttribute(attribute),
	m_ReadOnly(true)
{
}

KXMLAttribute::~KXMLAttribute()
{
}

bool KXMLAttribute::IsReadOnly() const
{
	return m_ReadOnly;
}

bool KXMLAttribute::IsEmpty() const
{
	if (IsReadOnly())
	{
		return m_ReadOnlyAttribute == nullptr;
	}
	else
	{
		return m_Attribute == nullptr;
	}
}

bool KXMLAttribute::BoolValue() const
{
	ASSERT_RESULT(!IsEmpty());
	return m_ReadOnlyAttribute->BoolValue();
}

int KXMLAttribute::IntValue() const
{
	ASSERT_RESULT(!IsEmpty());
	return m_ReadOnlyAttribute->IntValue();
}

float KXMLAttribute::FloatValue() const
{
	ASSERT_RESULT(!IsEmpty());
	return m_ReadOnlyAttribute->FloatValue();
}

std::string KXMLAttribute::Value() const
{
	ASSERT_RESULT(!IsEmpty());
	return m_ReadOnlyAttribute->Value();
}

void KXMLAttribute::SetAttribute(bool value)
{
	ASSERT_RESULT(!IsEmpty() && !IsReadOnly());
	m_Attribute->SetAttribute(value);
}

void KXMLAttribute::SetAttribute(float value)
{
	ASSERT_RESULT(!IsEmpty() && !IsReadOnly());
	m_Attribute->SetAttribute(value);
}

void KXMLAttribute::SetAttribute(double value)
{
	ASSERT_RESULT(!IsEmpty() && !IsReadOnly());
	m_Attribute->SetAttribute(value);
}

void KXMLAttribute::SetAttribute(unsigned int value)
{
	ASSERT_RESULT(!IsEmpty() && !IsReadOnly());
	m_Attribute->SetAttribute(value);
}

void KXMLAttribute::SetAttribute(int value)
{
	ASSERT_RESULT(!IsEmpty() && !IsReadOnly());
	m_Attribute->SetAttribute(value);
}

void KXMLAttribute::SetAttribute(const char* value)
{
	ASSERT_RESULT(!IsEmpty() && !IsReadOnly());
	m_Attribute->SetAttribute(value);
}

IKXMLAttributePtr KXMLAttribute::NextAttribute() const
{
	ASSERT_RESULT(!IsEmpty());
	return IKXMLAttributePtr(new KXMLAttribute(m_ReadOnlyAttribute->Next()));
}