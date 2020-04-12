#pragma once

#include <QHBoxLayout>
#include "ui_KEResourceBrowser.h"
#include "KBase/Interface/IKFileSystem.h"

struct FileSystemComboData : public QObjectUserData
{
	IKFileSystemPtr system;
};
Q_DECLARE_METATYPE(FileSystemComboData);

class KEFileSystemTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	struct FileSystemTreeItem
	{
		std::string name;
		std::string fullPath;
		std::vector<FileSystemTreeItem*> children;

		FileSystemTreeItem* parent;
		int index;
		bool isDir;

		FileSystemTreeItem(IKFileSystemPtr system,
			const std::string& _name,
			const std::string& _fullPath,
			FileSystemTreeItem* _parent,
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
				FileSystemTreeItem* newItem = nullptr;
				std::string fullSubPath;
				system->FullPath(fullPath, subPath, fullSubPath);
				newItem = new FileSystemTreeItem(system,
					subPath,
					fullSubPath,
					this,
					index,
					system->IsDir(fullSubPath));
				++index;

				children.push_back(newItem);
			}
		}

		~FileSystemTreeItem()
		{
			for (FileSystemTreeItem* item : children)
			{
				SAFE_DELETE(item);
			}
			children.clear();
		}

		FileSystemTreeItem* GetChild(size_t index)
		{
			if (index < children.size())
			{
				return children[index];
			}
			return nullptr;
		}
	};
protected:
	IKFileSystemPtr m_FileSys;
	FileSystemTreeItem* m_RootItem;
public:
	KEFileSystemTreeModel(QObject *parent = 0);
	~KEFileSystemTreeModel();

	bool Init(IKFileSystemPtr fileSys, const std::string& name);

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
	KEFileSystemTreeModel m_TreeModel;
protected slots:
	void OnComboIndexChanged(int index);
private:
	Ui::KEResourceBrowser ui;
};
