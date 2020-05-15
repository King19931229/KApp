#pragma once
#include "KBase/Interface/IKReflection.h"
#include "KEditorConfig.h"
#include "Property/KEPropertyBaseView.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

class KEReflectionManager;

class KEReflectObjectWidget : public QWidget
{
	Q_OBJECT
	friend class KEReflectionManager;
protected:
	void AddProperty(const std::string& name, KEPropertyBaseView::BasePtr propertyView);
	void AddObject(const std::string& name, KEReflectObjectWidget* widget);

	QVBoxLayout* m_Layout;

	struct PropertyItem
	{
		QLayout* layout;
		QLabel* label;
		std::string name;
		KEPropertyBaseView::BasePtr propertyView;

		PropertyItem()
		{
			layout = nullptr;
			label = nullptr;
		}
	};
	struct ObjectItem
	{
		std::string name;
		KEReflectObjectWidget* widget;

		ObjectItem()
		{
			widget = nullptr;
		}
	};

	std::list<PropertyItem> m_Properties;
	std::list<ObjectItem> m_Objects;
public:
	KEReflectObjectWidget(const std::string& name);
	~KEReflectObjectWidget();
};

class KEReflectPropertyWidget;

class KEReflectionManager
{
public:
	typedef std::unordered_map<KReflectionObjectBase*, KEReflectObjectWidget*> ObjectWidgetMap;
	ObjectWidgetMap m_WidgetMap;
	KEReflectPropertyWidget* m_PropertyWidget;

	KEReflectObjectWidget* Build(KReflectionObjectBase* object);
public:
	KEReflectionManager();
	~KEReflectionManager();

	bool Init(KEReflectPropertyWidget* widget);
	bool UnInit();

	bool Watch(KReflectionObjectBase* object);
	bool Discard(KReflectionObjectBase* object);
	bool Refresh(KReflectionObjectBase* obect);

	void SetCurrent(KReflectionObjectBase* object);
};