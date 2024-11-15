#pragma once

#include <QWidget>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include "ui_KESceneItemWidget.h"
#include "KEditorConfig.h"
#include <functional>

class KESceneItemModel : public QAbstractListModel
{
protected:
	friend class KESceneItemWidget;
	std::vector<KEEntityPtr> m_Entities;
	std::function<bool(KEEntityPtr, KEEntityPtr)> m_Compare;
public:
	KESceneItemModel(QObject *parent = Q_NULLPTR);
	~KESceneItemModel();

	bool Init();
	bool UnInit();

	void Add(KEEntityPtr entity);
	void Erase(KEEntityPtr entity);
	void Clear();

	bool GetIndex(KEEntityPtr entity, size_t& index);

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &child) const override;
};

class KESceneItemWidget : public QWidget
{
	Q_OBJECT
protected:
	friend class KESceneItemModel;
	Ui::Form ui;
	KESceneItemModel* m_Model;
	QSortFilterProxyModel* m_ProxyModel;
	void UpdateView();
public:
	KESceneItemWidget(QWidget *parent = Q_NULLPTR);
	~KESceneItemWidget();

	bool Init();
	bool UnInit();

	void Add(KEEntityPtr entity);
	void Remove(KEEntityPtr entity);
	void Clear();

	bool Select(KEEntityPtr entity, bool select);
	bool ClearSelection();
protected Q_SLOTS:
	void OnSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void OnSearchTextChanged(const QString& newText);
};