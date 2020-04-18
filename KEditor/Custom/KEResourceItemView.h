#pragma once
#include <QListView>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMimeData>
#include <QDrag>

class KEFileSystemTreeItem;
struct KEResourceItemDropData : public QObjectUserData
{
	KEFileSystemTreeItem* item;
	KEResourceItemDropData()
	{
		item = nullptr;
	}
	~KEResourceItemDropData()
	{
		item = nullptr;
	}
};

class KEResourceItemView : public QListView
{
public:
	KEResourceItemView(QWidget *parent = Q_NULLPTR);
	~KEResourceItemView();
	void mouseMoveEvent(QMouseEvent *event) override;
};