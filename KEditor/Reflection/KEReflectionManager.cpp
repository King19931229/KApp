#include "KEReflectionManager.h"
#include "Widget/KEReflectPropertyWidget.h"
#include <assert.h>

KEReflectionManager::KEReflectionManager()
	: m_PropertyWidget(nullptr)
{
}

KEReflectionManager::~KEReflectionManager()
{
	ASSERT_RESULT(m_PropertyWidget == nullptr);
}

bool KEReflectionManager::Init(KEReflectPropertyWidget* widget)
{
	m_PropertyWidget = widget;
	return true;
}

bool KEReflectionManager::UnInit()
{
	if (m_PropertyWidget)
	{
		m_PropertyWidget->SetObject(nullptr);
	}
	m_PropertyWidget = nullptr;
	return true;
}

bool KEReflectionManager::Refresh(KReflectionObjectBase* obect)
{
	return true;
}

void KEReflectionManager::SetCurrent(KReflectionObjectBase* object)
{
	assert(m_PropertyWidget);
	m_PropertyWidget->SetObject(object);
}