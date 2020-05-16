#include "KEGraphView.h"
#include "KEGraphScene.h"
#include "KEGraphRegistrar.h"
#include "KEditorConfig.h"

#include "Graph/Node/KEGraphNodeControl.h"
#include "Graph/Node/KEGraphNodeView.h"
#include "Graph/Node/KEGraphNodeModel.h"

#include "Graph/Connection/KEGraphConnectionControl.h"
#include "Graph/Connection/KEGraphConnectionView.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtWidgets/QMenu>
#include <QTreeWidget>
#include <QWidgetAction>
#include <QLineEdit>
#include <QHeaderView>

KEGraphView::KEGraphView(QWidget *parent)
	: QGraphicsView(parent),
	m_Scene(nullptr),
	m_ClearSelectionAction(nullptr),
	m_DeleteSelectionAction(nullptr)
{
	//setDragMode(QGraphicsView::ScrollHandDrag);
	setRenderHint(QPainter::Antialiasing);

	QBrush backgroundBrush = QBrush(QColor(53, 53, 53));
	setBackgroundBrush(backgroundBrush);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setCacheMode(QGraphicsView::CacheBackground);

	m_Scene = KEGraphScenePtr(KNEW KEGraphScene(this));
	this->setScene(m_Scene.get());

	m_ClearSelectionAction = QActionPtr(KNEW QAction(QStringLiteral("Clear Selection"), this));
	m_ClearSelectionAction->setShortcut(Qt::Key_Escape);
	connect(m_ClearSelectionAction.get(), &QAction::triggered, m_Scene.get(), &QGraphicsScene::clearSelection);
	addAction(m_ClearSelectionAction.get());

	m_DeleteSelectionAction = QActionPtr(KNEW QAction(QStringLiteral("Delete Selection"), this));
	m_DeleteSelectionAction->setShortcut(Qt::Key_Delete);
	connect(m_DeleteSelectionAction.get(), &QAction::triggered, this, &KEGraphView::DeleteSelectedNodes);
	addAction(m_DeleteSelectionAction.get());

	connect((KEGraphScene*)scene(),
		&KEGraphScene::SingalNodeContextMenu,
		this,
		&KEGraphView::OnNodeContextMenu);

	connect((KEGraphScene*)scene(),
		&KEGraphScene::SingalConnectionContextMenu,
		this,
		&KEGraphView::OnConnectionContextMenu);
}

KEGraphView::~KEGraphView()
{
}

void KEGraphView::DeleteSelectedNodes()
{
	// Delete the selected connections first, ensuring that they won't be
	// automatically deleted when selected nodes are deleted (deleting a node
	// deletes some connections as well)
	for (QGraphicsItem * item : m_Scene->selectedItems())
	{
		if (KEGraphConnectionView* c = qgraphicsitem_cast<KEGraphConnectionView*>(item))
		{
			m_Scene->DeleteConnection(c->Connection()->ID());
		}
	}

	// Delete the nodes; this will delete many of the connections.
	// Selected connections were already deleted prior to this loop, otherwise
	// qgraphicsitem_cast<NodeGraphicsObject*>(item) could be a use-after-free
	// when a selected connection is deleted by deleting the node.
	for (QGraphicsItem * item : m_Scene->selectedItems())
	{
		if (KEGraphNodeView* n = qgraphicsitem_cast<KEGraphNodeView*>(item))
		{
			auto node = n->GetControl();
			auto model = node->GetModel();
			if (model->Deletable())
			{
				m_Scene->RemoveNode(node->ID());
			}
		}
	}
}

void KEGraphView::OnNodeContextMenu(KEGraphNodeControl* n, const QPointF& pos)
{
	QMenu nodeMenu;

	QWidgetAction *treeAction = KNEW QWidgetAction(&nodeMenu);

	QTreeWidget *treeView = KNEW QTreeWidget(&nodeMenu);
	treeView->header()->close();

	treeAction->setDefaultWidget(treeView);
	nodeMenu.addAction(treeAction);

	const static QString copyOp = QString("Copy");
	const static QString pasteOp = QString("Paste");
	const static QString deleteOp = QString("Delete");

	for (const QString& op : { /*copyOp, pasteOp,*/ deleteOp })
	{
		QTreeWidgetItem* item = KNEW QTreeWidgetItem(treeView);
		item->setText(0, op);
		item->setData(0, Qt::UserRole, op);
	}

	connect(treeView, &QTreeWidget::itemClicked, [&](QTreeWidgetItem *item, int)
	{
		QString op = item->data(0, Qt::UserRole).toString();

		if (op == deleteOp)
		{
			auto model = n->GetModel();
			if (model->Deletable())
			{
				m_Scene->RemoveNode(n->ID());
			}
		}

		nodeMenu.close();
	});
	treeView->expandAll();

	nodeMenu.exec(QPoint(mapToGlobal(mapFromScene(pos))));
}

void KEGraphView::OnConnectionContextMenu(KEGraphConnectionControl* conn, const QPointF& pos)
{
	QMenu connMenu;

	QWidgetAction *treeAction = KNEW QWidgetAction(&connMenu);

	QTreeWidget *treeView = KNEW QTreeWidget(&connMenu);
	treeView->header()->close();

	treeAction->setDefaultWidget(treeView);
	connMenu.addAction(treeAction);

	const static QString copyOp = QString("Copy");
	const static QString pasteOp = QString("Paste");
	const static QString deleteOp = QString("Delete");

	for (const QString& op : { /*copyOp, pasteOp,*/ deleteOp })
	{
		QTreeWidgetItem* item = KNEW QTreeWidgetItem(treeView);
		item->setText(0, op);
		item->setData(0, Qt::UserRole, op);
	}

	connect(treeView, &QTreeWidget::itemClicked, [&](QTreeWidgetItem *item, int)
	{
		QString op = item->data(0, Qt::UserRole).toString();

		if (op == deleteOp)
		{
			m_Scene->DeleteConnection(conn->ID());
		}

		connMenu.close();
	});
	treeView->expandAll();

	connMenu.exec(QPoint(mapToGlobal(mapFromScene(pos))));
}

void KEGraphView::ScaleUp()
{
	double const step = 1.2;
	double const factor = std::pow(step, 1.0);

	QTransform t = transform();

	if (t.m11() > 2.0)
		return;

	scale(factor, factor);
}

void KEGraphView::ScaleDown()
{
	double const step = 1.2;
	double const factor = std::pow(step, -1.0);

	scale(factor, factor);
}

void KEGraphView::RegisterModel(const QString& name, KEGraphNodeModelCreateFunc func)
{
	m_Scene->GetRegistrar()->RegisterGraphModel(name, func);
}

void KEGraphView::UnRegisterGraphModel(const QString& name)
{
	m_Scene->GetRegistrar()->UnRegisterGraphModel(name);
}

void KEGraphView::contextMenuEvent(QContextMenuEvent *event)
{
	if (itemAt(event->pos()))
	{
		QGraphicsView::contextMenuEvent(event);
		return;
	}

	QMenu modelMenu;

	//Add filterbox to the context menu
	QLineEdit *txtBox = KNEW QLineEdit(&modelMenu);
	txtBox->setPlaceholderText(QStringLiteral("Filter"));
	txtBox->setClearButtonEnabled(true);

	QWidgetAction *txtBoxAction = KNEW QWidgetAction(&modelMenu);
	txtBoxAction->setDefaultWidget(txtBox);

	modelMenu.addAction(txtBoxAction);

	//Add result treeview to the context menu
	QTreeWidget *treeView = KNEW QTreeWidget(&modelMenu);
	treeView->header()->close();

	QWidgetAction *treeViewAction = KNEW QWidgetAction(&modelMenu);
	treeViewAction->setDefaultWidget(treeView);

	modelMenu.addAction(treeViewAction);

	m_Scene->GetRegistrar()->VisitModel([&](const QString& name, auto func)
	{
		QString testText = name;
		QTreeWidgetItem* item = KNEW QTreeWidgetItem(treeView);
		item->setText(0, name);
		item->setData(0, Qt::UserRole, name);
	});

	connect(treeView, &QTreeWidget::itemClicked, [&](QTreeWidgetItem *item, int)
	{
		QString modelName = item->data(0, Qt::UserRole).toString();
		KEGraphNodeModelPtr model = m_Scene->GetRegistrar()->GetNodeModel(modelName);
	
		if (model)
		{
			QPoint pos = event->pos();
			QPointF posView = this->mapToScene(pos);
			KEGraphNodeControl* nodeControl = m_Scene->CreateNode(std::move(model));
			nodeControl->GetView()->setPos(posView);
			m_Scene->SingalNodePlaced(nodeControl);
		}

		modelMenu.close();
	});

	treeView->expandAll();

	modelMenu.exec(event->globalPos());
}

void KEGraphView::wheelEvent(QWheelEvent *event)
{
	QPoint delta = event->angleDelta();

	if (delta.y() == 0)
	{
		event->ignore();
		return;
	}

	double const d = delta.y() / std::abs(delta.y());

	if (d > 0.0)
	{
		ScaleUp();
	}
	else
	{
		ScaleDown();
	}
}

void KEGraphView::mousePressEvent(QMouseEvent *event)
{
	QGraphicsView::mousePressEvent(event);
	if (event->button() == Qt::LeftButton)
	{
		m_ClickPos = mapToScene(event->pos());
	}
}

void KEGraphView::mouseMoveEvent(QMouseEvent *event)
{
	QGraphicsView::mouseMoveEvent(event);
	if (scene()->mouseGrabberItem() == nullptr && event->buttons() == Qt::LeftButton)
	{
		// Make sure shift is not being pressed
		if ((event->modifiers() & Qt::ShiftModifier) == 0)
		{
			QPointF difference = m_ClickPos - mapToScene(event->pos());
			setSceneRect(sceneRect().translated(difference.x(), difference.y()));
		}
	}
}

void KEGraphView::drawBackground(QPainter* painter, const QRectF& r)
{
	QGraphicsView::drawBackground(painter, r);

	auto drawGrid =	[&](double gridStep)
	{
		QRect   windowRect = rect();
		QPointF tl = mapToScene(windowRect.topLeft());
		QPointF br = mapToScene(windowRect.bottomRight());

		double left = std::floor(tl.x() / gridStep - 0.5);
		double right = std::floor(br.x() / gridStep + 1.0);
		double bottom = std::floor(tl.y() / gridStep - 0.5);
		double top = std::floor(br.y() / gridStep + 1.0);

		// vertical lines
		for (int xi = int(left); xi <= int(right); ++xi)
		{
			QLineF line(xi * gridStep, bottom * gridStep, xi * gridStep, top * gridStep);
			painter->drawLine(line);
		}

		// horizontal lines
		for (int yi = int(bottom); yi <= int(top); ++yi)
		{
			QLineF line(left * gridStep, yi * gridStep, right * gridStep, yi * gridStep);
			painter->drawLine(line);
		}
	};

	QPen pfine(QBrush(QColor(60, 60, 60)), 1.0);
	painter->setPen(pfine);
	drawGrid(15);	
	QPen p(QBrush(QColor(25, 25, 25)), 1.0);
	painter->setPen(p);
	drawGrid(150);
}

void KEGraphView::showEvent(QShowEvent *event)
{
	m_Scene->setSceneRect(this->rect());
	QGraphicsView::showEvent(event);
}