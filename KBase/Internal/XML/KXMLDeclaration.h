#pragma once
#include "Interface/IKXML.h"
#include "tinyxml2.h"

class KXMLDeclaration : public IKXMLDeclaration
{
	friend class KXMLDocument;
protected:
	tinyxml2::XMLDeclaration* m_Declaration;
public:
	KXMLDeclaration(tinyxml2::XMLDeclaration* decl);
	virtual ~KXMLDeclaration();

	bool IsEmpty() override;
	void SetValue(const char* value) override;
	std::string Value() const override;
};