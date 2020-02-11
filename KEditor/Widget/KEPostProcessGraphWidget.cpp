#include "KEPostProcessGraphWidget.h"
#include "PostProcess/KEPostProcessGraphView.h"
#include "PostProcess/KEPostProcessPassModel.h"
#include "PostProcess/KEPostProcessTextureModel.h"

KEPostProcessGraphWidget::KEPostProcessGraphWidget()
	: KEGraphWidget()
{	
}

KEPostProcessGraphWidget::~KEPostProcessGraphWidget()
{
}

KEGraphView* KEPostProcessGraphWidget::CreateViewImpl()
{
	return new KEPostProcessGraphView();
}

bool KEPostProcessGraphWidget::Init()
{
	if (KEGraphWidget::Init())
	{
		m_View->RegisterModel("Pass", []()->KEGraphNodeModelPtr
		{
			return KEGraphNodeModelPtr(new KEPostProcessPassModel(GetProcessManager()->CreatePass()));
		});

		/*m_View->RegisterModel("Texture", []()->KEGraphNodeModelPtr
		{
			return KEGraphNodeModelPtr(new KEPostProcessTextureModel());
		});*/

		QAction* syncAction = m_MenuBar->addAction("Sync");
		connect(syncAction, &QAction::triggered, this, &KEPostProcessGraphWidget::Sync);

		QAction* autoLayoutAction = m_MenuBar->addAction("AutoLayout");
		connect(autoLayoutAction, &QAction::triggered, this, &KEPostProcessGraphWidget::AutoLayout);

		return true;
	}
	return false;
}

bool KEPostProcessGraphWidget::UnInit()
{
	return KEGraphWidget::UnInit();
}

void KEPostProcessGraphWidget::Sync()
{
	KEPostProcessGraphView* view = static_cast<KEPostProcessGraphView*>(m_View);
	view->Sync();
}

void KEPostProcessGraphWidget::AutoLayout()
{
	KEPostProcessGraphView* view = static_cast<KEPostProcessGraphView*>(m_View);
	view->AutoLayout();
}