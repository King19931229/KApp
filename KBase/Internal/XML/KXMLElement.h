#pragma once
#include "Interface/IKXML.h"
#include "tinyxml2.h"

class KXMLElement : public IKXMLElement
{
	friend class KXMLDocument;
protected:
	tinyxml2::XMLElement* m_Element;
public:
	KXMLElement(tinyxml2::XMLElement* element);
	virtual ~KXMLElement();

	virtual bool IsEmpty() const override;
	virtual bool HasNextSiblingElement() const override;

	virtual IKXMLElementPtr NextSiblingElement(const char* name) const override;
	virtual IKXMLElementPtr FirstChildElement(const char* name) const override;

	virtual std::string GetValue() const override;
	virtual std::string GetText() const override;
	virtual void SetText(const char* text) override;
	virtual void SetText(int value) override;
	virtual void SetText(unsigned int value) override;
	virtual void SetText(bool value) override;

	virtual IKXMLAttributePtr FirstAttribute() const override;
	virtual IKXMLAttributePtr FindAttribute(const char* attribute) const override;

	virtual void DeleteAttribute(const char* attribute) override;
	virtual void SetAttribute(const char* attribute, bool value) override;
	virtual void SetAttribute(const char* attribute, float value) override;
	virtual void SetAttribute(const char* attribute, double value) override;
	virtual void SetAttribute(const char* attribute, unsigned int value) override;
	virtual void SetAttribute(const char* attribute, int value) override;
	virtual void SetAttribute(const char* attribute, const char* value) override;

	virtual IKXMLElementPtr NewElement(const char* element, bool linkAtOnce) override;
	virtual void LinkElementToEnd(IKXMLElementPtr& element) override;
};