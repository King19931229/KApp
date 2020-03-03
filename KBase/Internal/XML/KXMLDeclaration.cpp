#include "KXMLDeclaration.h"
#include <assert.h>

KXMLDeclaration::KXMLDeclaration(tinyxml2::XMLDeclaration* decl)
: m_Declaration(decl)
{}

KXMLDeclaration::~KXMLDeclaration()
{

}

bool KXMLDeclaration::IsEmpty()
{
	return m_Declaration == nullptr;
}

void KXMLDeclaration::SetValue(const char* value)
{
	ASSERT_RESULT(m_Declaration);
	m_Declaration->SetValue(value);
}

std::string KXMLDeclaration::Value() const
{
	ASSERT_RESULT(m_Declaration);
	return m_Declaration->Value();
}