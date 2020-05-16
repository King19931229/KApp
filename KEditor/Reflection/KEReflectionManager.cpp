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
	m_PropertyWidget = nullptr;
	return true;
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

void KEReflectionManager::Add(KReflectionObjectBase* object)
{
	assert(m_PropertyWidget);
	m_PropertyWidget->AddObject(object);
}

void KEReflectionManager::Remove(KReflectionObjectBase* object)
{
	assert(m_PropertyWidget);
	m_PropertyWidget->RemoveObject(object);
}