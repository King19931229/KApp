#pragma once
#include "Interface/IKXML.h"
#include "tinyxml2.h"

class KXMLDocument : public IKXMLDocument
{
protected:
	tinyxml2::XMLDocument *m_Document;		
public:
	KXMLDocument();
	~KXMLDocument();
	bool SaveFile(const char* fileName) override;
	bool ParseFromFile(const char* fileName) override;
	bool ParseFromString(const char* text) override;
	IKXMLDeclarationPtr NewDeclaration(const char* decl, bool linkAtOnce) override;
	IKXMLElementPtr NewElement(const char* element, bool linkAtOnce) override;
	bool HasChildrenElement() const override;
	IKXMLElementPtr FirstChildElement(const char* name) const override;
	void LinkElementToEnd(IKXMLElementPtr& element) override;
	void LinkDeclarationToEnd(IKXMLDeclarationPtr& decl) override;
};