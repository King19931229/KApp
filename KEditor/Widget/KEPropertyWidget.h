#pragma once
#include "KEditorConfig.h"
#include "Property/KEPropertyBaseView.h"
#include <QWidget>
#include <QLabel>
#include <list>

class KEPropertyWidget : public QWidget
{
	Q_OBJECT
protected:
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
	std::list<PropertyItem> m_Items;
	QVBoxLayout* m_Layout;
	bool m_bInit;
public:
	KEPropertyWidget();
	~KEPropertyWidget();

	bool Init();
	bool UnInit();

	bool AppendItem(const std::string& name, KEPropertyBaseView::BasePtr propertyView);
	bool RemoveItem(const std::string& name);
};