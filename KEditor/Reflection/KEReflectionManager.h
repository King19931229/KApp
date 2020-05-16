#pragma once
#include "KBase/Interface/IKReflection.h"
#include "KEditorConfig.h"
#include "Property/KEPropertyBaseView.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

class KEReflectionManager;
class KEReflectPropertyWidget;

class KEReflectionManager
{
public:
	KEReflectPropertyWidget* m_PropertyWidget;
public:
	KEReflectionManager();
	~KEReflectionManager();

	bool Init(KEReflectPropertyWidget* widget);
	bool UnInit();

	// 内存数据改变 通知回属性面板作出改变
	void NotifyToProperty(KReflectionObjectBase* object);
	// 属性面板数据改变 通知回编辑器作出改变(包括Gizmo位置等等)
	void NotifyToEditor(KReflectionObjectBase* object);

	void Add(KReflectionObjectBase* object);
	void Remove(KReflectionObjectBase* object);
};