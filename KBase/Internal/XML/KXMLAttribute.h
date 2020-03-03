#pragma once
#include "Interface/IKXML.h"
#include "tinyxml2.h"

class KXMLAttribute : public IKXMLAttribute
{
protected:
	tinyxml2::XMLAttribute *m_Attribute;
	const tinyxml2::XMLAttribute *m_ReadOnlyAttribute;
	bool m_ReadOnly;
public:
	KXMLAttribute();
	KXMLAttribute(tinyxml2::XMLAttribute* attribute);
	KXMLAttribute(const tinyxml2::XMLAttribute* attribute);
	~KXMLAttribute();

	bool IsReadOnly() const override;
	bool IsEmpty() const override;

	bool BoolValue() const override;
	int IntValue() const override;
	float FloatValue() const override;
	std::string Value() const override;

	void SetAttribute(bool value) override;
	void SetAttribute(float value) override;
	void SetAttribute(double value) override;
	void SetAttribute(unsigned int value) override;
	void SetAttribute(int value) override;
	void SetAttribute(const char* value) override;

	IKXMLAttributePtr NextAttribute() const override;
};