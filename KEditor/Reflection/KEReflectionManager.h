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

	bool Refresh(KReflectionObjectBase* obect);
	void SetCurrent(KReflectionObjectBase* object);
};