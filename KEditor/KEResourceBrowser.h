#pragma once

#include <QHBoxLayout>
#include <QListView>
#include "ui_KEResourceBrowser.h"
#include "KBase/Interface/IKFileSystem.h"

struct KEFileSystemComboData : public QObjectUserData
{
	IKFileSystemPtr system;
};
Q_DECLARE_METATYPE(KEFileSystemComboData);

struct KEFileSystemTreeItem
{
	std::string name;
	std::string fullPath;
	std::vector<KEFileSystemTreeItem*> children;

	KEFileSystemTreeItem* parent;
	int index;
	bool isDir;

	KEFileSystemTreeItem(IKFileSystemPtr system,
		const std::string& _name,
		const std::string& _fullPath,
		KEFileSystemTreeItem* _parent,
		int _index,
		bool _isDir)
	{
		name = _name;
		fullPath = _fullPath;
		parent = _parent;
		index = _index;
		isDir = _isDir;

		std::vector<std::string> listDir;
		system->ListDir(fullPath, listDir);

		int index = 0;
		for (const std::string& subPath : listDir)
		{
			KEFileSystemTreeItem* newItem = nullptr;
			std::string fullSubPath;
			system->FullPath(fullPath, subPath, fullSubPath);
			newItem = new KEFileSystemTreeItem(system,
				subPath,
				fullSubPath,
				this,
				index,
				system->IsDir(fullSubPath));
			++index;

			children.push_back(newItem);
		}
	}

	~KEFileSystemTreeItem()
	{
		for (KEFileSystemTreeItem* item : children)
		{
			SAFE_DELETE(item);
		}
		children.clear();
	}

	KEFileSystemTreeItem* GetChild(size_t index)
	{
		if (index < children.size())
		{
			return children[index];
		}
		return nullptr;
	}
};

class KEFileSystemTreeModel : public QAbstractItemModel
{
	Q_OBJECT
protected:
	IKFileSystemPtr m_FileSys;
	KEFileSystemTreeItem* m_Item;
public:
	KEFileSystemTreeModel(QObject *parent = 0);
	~KEFileSystemTreeModel();

	void SetItem(KEFileSystemTreeItem* item);
	KEFileSystemTreeItem* GetItem();

	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column,
		const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
};

class KEResourceBrowser : public QWidget
{
	Q_OBJECT
public:
	KEResourceBrowser(QWidget *parent = Q_NULLPTR);
	~KEResourceBrowser();

	QSize sizeHint() const override;

	bool Init();
	bool UnInit();
protected:
	QWidget* m_MainWindow;
	KEFileSystemTreeItem* m_RootItem;
	KEFileSystemTreeModel m_TreeModel;
protected Q_SLOTS:
	void OnComboIndexChanged(int index);
	void OnTreeViewClicked(QModelIndex index);
	void OnTreeViewBack(bool);
private:
	Ui::KEResourceBrowser ui;
};
