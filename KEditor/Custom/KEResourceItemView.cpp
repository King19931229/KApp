#include "KEResourceItemView.h"
#include "Browser/KEFileSystemTreeItem.h"

KEResourceItemView::KEResourceItemView(QWidget *parent)
	: QListView(parent)
{
}

KEResourceItemView::~KEResourceItemView()
{
}

void KEResourceItemView::mouseMoveEvent(QMouseEvent *event)
{
	QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
	if (selectedIndexes.size() > 0)
	{
		QModelIndex index = selectedIndexes[0];
		KEFileSystemTreeItem* treeItem = (KEFileSystemTreeItem*)index.internalPointer();
		QMimeData* mimeData = new QMimeData();

		KEResourceItemDropData* dropData = new KEResourceItemDropData();
		dropData->item = treeItem;

		QDrag* drag = new QDrag(this);
		drag->setMimeData(mimeData);
		mimeData->setUserData(0, dropData);

		Qt::DropAction action = drag->exec(Qt::MoveAction);
		event->ignore();
	}
	QListView::mouseMoveEvent(event);
}