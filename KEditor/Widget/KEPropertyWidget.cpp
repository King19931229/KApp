#include "KEPropertyWidget.h"
#include <algorithm>

KEPropertyWidget::KEPropertyWidget()
	: m_Layout(nullptr),
	m_bInit(false)
{
}

KEPropertyWidget::~KEPropertyWidget()
{
	assert(!m_Layout);
	assert(!m_bInit);
}

bool KEPropertyWidget::Init()
{
	if (!m_bInit)
	{
		m_Layout = new QVBoxLayout();
		setLayout(m_Layout);
		m_bInit = true;
		return true;
	}
	return false;
}

bool KEPropertyWidget::UnInit()
{
	if (m_bInit)
	{
		for (auto it = m_Items.begin(), itEnd = m_Items.end();
			it != itEnd; ++it)
		{
			m_Layout->removeItem(it->layout);

			it->layout->removeWidget(it->label);
			it->layout->removeItem(it->propertyView->GetLayout());

			SAFE_DELETE(it->layout);
			SAFE_DELETE(it->label);
		}
		m_Items.clear();

		SAFE_DELETE(m_Layout);

		m_bInit = false;
	}
	return true;
}

bool KEPropertyWidget::AppendItem(const std::string& name, KEPropertyBaseView::BasePtr propertyView)
{
	assert(m_bInit);
	auto it = std::find_if(m_Items.begin(), m_Items.end(), [&name](const PropertyItem& item)->bool
	{
		return item.name == name;
	});

	if (it == m_Items.end())
	{
		PropertyItem newItem;

		newItem.layout = new QHBoxLayout();
		newItem.label = new QLabel(name.c_str());
		newItem.name = name;
		newItem.propertyView = propertyView;

		static_cast<QHBoxLayout*>(newItem.layout)->addWidget(newItem.label);
		static_cast<QHBoxLayout*>(newItem.layout)->addLayout(propertyView->GetLayout());

		m_Layout->addLayout(newItem.layout);

		m_Items.push_back(std::move(newItem));
		return true;
	}
	else
	{
		return false;
	}
}

bool KEPropertyWidget::RemoveItem(const std::string& name)
{
	assert(m_bInit);
	auto it = std::find_if(m_Items.begin(), m_Items.end(), [&name](const PropertyItem& item)->bool
	{
		return item.name == name;
	});

	if (it != m_Items.end())
	{
		m_Layout->removeItem(it->layout);

		it->layout->removeWidget(it->label);
		it->layout->removeItem(it->propertyView->GetLayout());

		SAFE_DELETE(it->layout);
		SAFE_DELETE(it->label);
		return true;
	}
	else
	{
		return false;
	}
}