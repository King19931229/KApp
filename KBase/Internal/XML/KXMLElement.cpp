#include "KXMLElement.h"
#include "KXMLAttribute.h"
#include <assert.h>

KXMLElement::KXMLElement(tinyxml2::XMLElement* element)
	: m_Element(element)
{
}

KXMLElement::~KXMLElement()
{
}

bool KXMLElement::IsEmpty() const
{
	return m_Element == nullptr;
}

bool KXMLElement::HasNextSiblingElement() const
{
	if (IsEmpty())
	{
		return false;
	}
	return m_Element->NextSiblingElement() != nullptr;
}

IKXMLElementPtr KXMLElement::NextSiblingElement(const char* name) const
{
	if (IsEmpty())
	{
		return IKXMLElementPtr(new KXMLElement(nullptr));
	}
	return IKXMLElementPtr(new KXMLElement(m_Element->NextSiblingElement(name)));
}

IKXMLElementPtr KXMLElement::FirstChildElement(const char* name) const
{
	if (IsEmpty())
	{
		return IKXMLElementPtr(new KXMLElement(nullptr));
	}
	return IKXMLElementPtr(new KXMLElement(m_Element->FirstChildElement(name)));
}

std::string KXMLElement::GetValue() const
{
	ASSERT_RESULT(!IsEmpty());
	return m_Element->Value();
}

std::string KXMLElement::GetText() const
{
	ASSERT_RESULT(!IsEmpty());
	return m_Element->GetText();
}

void KXMLElement::SetText(const char* text)
{
	ASSERT_RESULT(!IsEmpty());
	m_Element->SetText(text);
}

void KXMLElement::SetText(int value)
{
	ASSERT_RESULT(!IsEmpty());
	m_Element->SetText(value);
}

IKXMLAttributePtr KXMLElement::FirstAttribute() const
{
	ASSERT_RESULT(!IsEmpty());
	return IKXMLAttributePtr(new KXMLAttribute(m_Element->FirstAttribute()));
}

IKXMLAttributePtr KXMLElement::FindAttribute(const char* attribute) const
{
	ASSERT_RESULT(!IsEmpty());
	return IKXMLAttributePtr(new KXMLAttribute(m_Element->FindAttribute(attribute)));
}

void KXMLElement::DeleteAttribute(const char* attribute)
{
	ASSERT_RESULT(!IsEmpty());
	m_Element->DeleteAttribute(attribute);
}

void KXMLElement::SetAttribute(const char* attribute, bool value)
{
	ASSERT_RESULT(!IsEmpty());
	m_Element->SetAttribute(attribute, value);
}

void KXMLElement::SetAttribute(const char* attribute, float value)
{
	ASSERT_RESULT(!IsEmpty());
	m_Element->SetAttribute(attribute, value);
}

void KXMLElement::SetAttribute(const char* attribute, double value)
{
	ASSERT_RESULT(!IsEmpty());
	m_Element->SetAttribute(attribute, value);
}

void KXMLElement::SetAttribute(const char* attribute, unsigned int value)
{
	ASSERT_RESULT(!IsEmpty());
	m_Element->SetAttribute(attribute, value);
}

void KXMLElement::SetAttribute(const char* attribute, int value)
{
	ASSERT_RESULT(!IsEmpty());
	m_Element->SetAttribute(attribute, value);
}

void KXMLElement::SetAttribute(const char* attribute, const char* value)
{
	ASSERT_RESULT(!IsEmpty());
	m_Element->SetAttribute(attribute, value);
}

IKXMLElementPtr KXMLElement::NewElement(const char* element, bool linkAtOnce)
{
	ASSERT_RESULT(!IsEmpty());
	tinyxml2::XMLElement* pEle = m_Element->GetDocument()->NewElement(element);
	if (linkAtOnce)
	{
		m_Element->LinkEndChild(pEle);
	}
	return IKXMLElementPtr(new KXMLElement(pEle));
}

void KXMLElement::LinkElementToEnd(IKXMLElementPtr& element)
{
	ASSERT_RESULT(!IsEmpty());
	ASSERT_RESULT(element->IsEmpty());

	assert(m_Element->GetDocument() == ((KXMLElement*)element.get())->m_Element->GetDocument());
	m_Element->LinkEndChild(((KXMLElement*)element.get())->m_Element);
}