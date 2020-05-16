#pragma once
#include <QAbstractItemModel>
#include "KEReflectionObjectItem.h"
#include <unordered_set>

class KEReflectObjectTreeModel : public QAbstractItemModel
{
	Q_OBJECT
protected:
	KEReflectionObjectItem* m_RootItem;
	std::unordered_set<KReflectionObjectBase*> m_Reflections;
public:
	KEReflectObjectTreeModel(QObject *parent = nullptr);
	~KEReflectObjectTreeModel();

	void AddReflection(KReflectionObjectBase* reflection);
	void RemoveReflection(KReflectionObjectBase* reflection);
	void RefreshReflection(KReflectionObjectBase* reflection);

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