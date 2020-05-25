#include "KXMLDocument.h"
#include "KXMLDeclaration.h"
#include "KXMLElement.h"
#include <fstream>
#include <strstream>

EXPORT_DLL IKXMLDocumentPtr GetXMLDocument()
{
	return IKXMLDocumentPtr(KNEW KXMLDocument());
}

KXMLDocument::KXMLDocument()
{
	m_Document = new tinyxml2::XMLDocument();
}

KXMLDocument::~KXMLDocument()
{
	SAFE_DELETE(m_Document);
}

bool KXMLDocument::SaveFile(const char* fileName)
{
	bool ret = (tinyxml2::XML_SUCCESS == m_Document->SaveFile(fileName));
	return ret;
}

bool KXMLDocument::ParseFromFile(const char* fileName)
{
	std::string str;

	std::ifstream infile(fileName, std::ios::trunc);

	infile.open(fileName, std::ifstream::in);
	if (infile.is_open())
	{
		infile.seekg(0, std::ios::end);
		str.reserve(infile.tellg());
		infile.seekg(0, std::ios::beg);

		str.assign((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
		infile.close();

		return ParseFromString(str.c_str());
	}
	else
	{
		return false;
	}
}

bool KXMLDocument::ParseFromString(const char* text)
{
	bool ret = (tinyxml2::XML_SUCCESS == m_Document->Parse(text));
	return ret;
}

IKXMLDeclarationPtr KXMLDocument::NewDeclaration(const char* decl, bool linkAtOnce)
{
	tinyxml2::XMLDeclaration* pDecl = m_Document->NewDeclaration(decl);
	if (linkAtOnce)
	{
		m_Document->LinkEndChild(pDecl);
	}
	return IKXMLDeclarationPtr(KNEW KXMLDeclaration(pDecl));
}

IKXMLElementPtr KXMLDocument::NewElement(const char* element, bool linkAtOnce)
{
	tinyxml2::XMLElement *pEle = m_Document->NewElement(element);
	if (linkAtOnce)
	{
		m_Document->LinkEndChild(pEle);
	}
	return IKXMLElementPtr(KNEW KXMLElement(pEle));
}

bool KXMLDocument::HasChildrenElement() const
{
	return m_Document->FirstChildElement() != nullptr;
}

IKXMLElementPtr KXMLDocument::FirstChildElement(const char* name) const
{
	return IKXMLElementPtr(KNEW KXMLElement(m_Document->FirstChildElement(name)));
}

void KXMLDocument::LinkElementToEnd(IKXMLElementPtr& element)
{
	m_Document->LinkEndChild(((KXMLElement*)element.get())->m_Element);
}

void KXMLDocument::LinkDeclarationToEnd(IKXMLDeclarationPtr& decl)
{
	m_Document->LinkEndChild(((KXMLDeclaration*)decl.get())->m_Declaration);
}