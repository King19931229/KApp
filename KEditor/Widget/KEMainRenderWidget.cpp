#include "KEMainRenderWidget.h"
#include "Other/KEQtRenderWindow.h"
#include "Browser/KEFileSystemTreeItem.h"

#include "Custom/KEResourceItemView.h"
#include "KEngine/Interface/IKEngine.h"
#include "KEditorGlobal.h"

#include <QWheelEvent>
#include <assert.h>

KEMainRenderWidget::KEMainRenderWidget(QWidget* pParent)
	: KERenderWidget(pParent)
{
}

KEMainRenderWidget::~KEMainRenderWidget()
{
}

bool KEMainRenderWidget::Init(IKEnginePtr& engine)
{
	if (engine)
	{
		m_Engine = engine.get();
		IKRenderCore* render = m_Engine->GetRenderCore();
		if (render)
		{
			m_RenderWindow = (KEQtRenderWindow*)(render->GetRenderWindow());
			m_RenderDevice = render->GetRenderDevice();
			return true;
		}
	}
	return false;
}

bool KEMainRenderWidget::UnInit()
{
	m_Engine = nullptr;
	m_RenderDevice = nullptr;
	m_RenderWindow = nullptr;
	return true;
}

void KEMainRenderWidget::paintEvent(QPaintEvent *event)
{
	m_Engine->Tick();
	KERenderWidget::paintEvent(event);
}

void KEMainRenderWidget::keyPressEvent(QKeyEvent *event)
{
	KERenderWidget::keyPressEvent(event);

	if (event->key() == Qt::Key_Z && event->modifiers() == Qt::CTRL)
	{
		// TODO Domain
		KEditorGlobal::CommandInvoker.Undo();
	}
	if (event->key() == Qt::Key_Y && event->modifiers() == Qt::CTRL)
	{
		// TODO Domain
		KEditorGlobal::CommandInvoker.Redo();
	}

	// TODO 临时硬代码
	if (event->key() == Qt::Key_J)
	{
		KEditorGlobal::EntityManipulator.Save("test.scene");
	}
	if (event->key() == Qt::Key_K)
	{
		KEditorGlobal::EntityManipulator.Load("test.scene");
	}
}

void KEMainRenderWidget::keyReleaseEvent(QKeyEvent *event)
{
	KERenderWidget::keyReleaseEvent(event);
}

void KEMainRenderWidget::dropEvent(QDropEvent *event)
{
	QObjectUserData* userData = event->mimeData()->userData(0);
	KEResourceItemDropData* resItemData = dynamic_cast<KEResourceItemDropData*>(userData);
	if (resItemData)
	{
		KEFileSystemTreeItem* item = resItemData->item;
		std::string fullPath = item->GetFullPath();
		IKEnginePtr engine = KEngineGlobal::Engine;
		IKRenderCore* renderCore = engine->GetRenderCore();
		IKEntityPtr entity = KEditorGlobal::ResourcePorter.Drop(renderCore->GetCamera(), fullPath);
		if (entity)
		{
			KEditorGlobal::EntityManipulator.Join(entity, fullPath);
		}
	}
}