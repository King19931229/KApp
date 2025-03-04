#pragma once
#include "KEditorConfig.h"
#include <QMainWindow>
#include <QVBoxLayout>

class KEReflectObjectTreeView;
class KEReflectObjectTreeModel;

class KEReflectPropertyWidget : public QMainWindow
{
	Q_OBJECT
protected:
	QWidget* m_MainWindow;
	KEReflectObjectTreeView* m_TreeView;
	KEReflectObjectTreeModel* m_TreeModel;
public:
	KEReflectPropertyWidget(QWidget *parent = Q_NULLPTR);
	~KEReflectPropertyWidget();

	QSize sizeHint() const override;

	void RefreshObject(KReflectionObjectBase* reflection);
	void AddObject(KReflectionObjectBase* reflection);
	void RemoveObject(KReflectionObjectBase* reflection);

	bool Init();
	bool UnInit();
};