#include "KEPostProcessGraphWidget.h"
#include "Graph/KEGraphView.h"
#include "PostProcess/KEPostProcessPassModel.h"
#include "PostProcess/KEPostProcessTextureModel.h"
#include "KRender/Interface/IKPostProcess.h"

KEPostProcessGraphWidget::KEPostProcessGraphWidget()
	: KEGraphWidget()
{
	m_View->RegisterModel("Pass", []()->KEGraphNodeModelPtr
	{
		return KEGraphNodeModelPtr(new KEPostProcessPassModel());
	});
	m_View->RegisterModel("Texture", []()->KEGraphNodeModelPtr
	{
		return KEGraphNodeModelPtr(new KEPostProcessTextureModel());
	});

	QAction* syncAction = m_MenuBar->addAction("Sync");
	connect(syncAction, &QAction::triggered, this, &KEPostProcessGraphWidget::SyncPostprocess);
}

KEPostProcessGraphWidget::~KEPostProcessGraphWidget()
{
}

void KEPostProcessGraphWidget::SyncPostprocess()
{
	IKPostProcessManager* manager = GetProcessManager();
	if (manager)
	{

	}
}