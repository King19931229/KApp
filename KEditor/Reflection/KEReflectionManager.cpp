#include "KEReflectionManager.h"
#include "Widget/KEReflectPropertyWidget.h"
#include "KEditorGlobal.h"
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

void KEReflectionManager::ClearProperty(KReflectionObjectBase* object)
{
	assert(m_PropertyWidget);
	m_PropertyWidget->ClearObject(object);
}

void KEReflectionManager::NotifyToProperty(KReflectionObjectBase* object)
{
	assert(m_PropertyWidget);
	m_PropertyWidget->RefreshObject(object);
}

void KEReflectionManager::NotifyToEditor(KReflectionObjectBase* object)
{
	KEditorGlobal::EntityManipulator.UpdateGizmoTransform();
}

void KEReflectionManager::SetCurrent(KReflectionObjectBase* object)
{
	assert(m_PropertyWidget);
	m_PropertyWidget->SetObject(object);
}