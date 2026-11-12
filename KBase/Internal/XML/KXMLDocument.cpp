#include "KXMLDocument.h"
#include "KXMLDeclaration.h"
#include "KXMLElement.h"
#include <fstream>
#include <strstream>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>

static FILE* OpenXMLFile(const char* fileName, const wchar_t* mode)
{
	if (!fileName) return nullptr;
	UINT codePage = CP_UTF8;
	DWORD flags = MB_ERR_INVALID_CHARS;
	int length = MultiByteToWideChar(codePage, flags, fileName, -1, nullptr, 0);
	if (!length)
	{
		codePage = CP_ACP;
		flags = 0;
		length = MultiByteToWideChar(codePage, flags, fileName, -1, nullptr, 0);
	}
	if (!length) return nullptr;
	std::wstring path(length, L'\0');
	MultiByteToWideChar(codePage, flags, fileName, -1, &path[0], length);
	FILE* file = nullptr;
	_wfopen_s(&file, path.c_str(), mode);
	return file;
}
#endif

EXPORT_DLL IKXMLDocumentPtr GetXMLDocument()
{
	return IKXMLDocumentPtr(KNEW KXMLDocument());
}

KXMLDocument::KXMLDocument()
{
	m_Document = KNEW tinyxml2::XMLDocument();
}

KXMLDocument::~KXMLDocument()
{
	SAFE_DELETE(m_Document);
}

bool KXMLDocument::SaveFile(const char* fileName)
{
#ifdef _WIN32
	FILE* file = OpenXMLFile(fileName, L"wb");
	if (!file) return false;
	bool ret = (tinyxml2::XML_SUCCESS == m_Document->SaveFile(file, false));
	return fclose(file) == 0 && ret;
#else
	bool ret = (tinyxml2::XML_SUCCESS == m_Document->SaveFile(fileName, false));
	return ret;
#endif
}

bool KXMLDocument::ParseFromFile(const char* fileName)
{
#ifdef _WIN32
	FILE* file = OpenXMLFile(fileName, L"rb");
	if (!file) return false;
	bool ret = (tinyxml2::XML_SUCCESS == m_Document->LoadFile(file));
	fclose(file);
	return ret;
#else
	std::string str;

	std::ifstream infile(fileName, std::ios::trunc);

	infile.open(fileName, std::ifstream::in);
	if (infile.is_open())
	{
		infile.seekg(0, std::ios::end);
		str.reserve((size_t)infile.tellg());
		infile.seekg(0, std::ios::beg);

		str.assign((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
		infile.close();

		return ParseFromString(str.c_str());
	}
	else
	{
		return false;
	}
#endif
}

bool KXMLDocument::ParseFromString(const char* text)
{
	tinyxml2::XMLError errorCode = m_Document->Parse(text);
	bool ret = (tinyxml2::XML_SUCCESS == errorCode);
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