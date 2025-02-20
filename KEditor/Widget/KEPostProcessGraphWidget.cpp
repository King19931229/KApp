#include "KEPostProcessGraphWidget.h"
#include "PostProcess/KEPostProcessGraphView.h"
#include "PostProcess/KEPostProcessPassModel.h"
#include "PostProcess/KEPostProcessTextureModel.h"

KEPostProcessGraphWidget::KEPostProcessGraphWidget(QWidget* parent)
	: KEGraphWidget(parent)
{
}

KEPostProcessGraphWidget::~KEPostProcessGraphWidget()
{
}

KEGraphView* KEPostProcessGraphWidget::CreateViewImpl()
{
	return KNEW KEPostProcessGraphView();
}

bool KEPostProcessGraphWidget::Init()
{
	if (KEGraphWidget::Init())
	{
		auto autoLayout = m_MenuBar->addAction("AutoLayout");
		connect(autoLayout, &QAction::triggered, this, [this]()
		{
			KEPostProcessGraphView* view = static_cast<KEPostProcessGraphView*>(m_View);
			view->AutoLayout();
		});

		auto Construct = m_MenuBar->addAction("Construct");
		connect(Construct, &QAction::triggered, this, [this]()
		{
			KEPostProcessGraphView* view = static_cast<KEPostProcessGraphView*>(m_View);
			view->Construct();
		});

		return true;
	}
	return false;
}

bool KEPostProcessGraphWidget::UnInit()
{
	return KEGraphWidget::UnInit();
}