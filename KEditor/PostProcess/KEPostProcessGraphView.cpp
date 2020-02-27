#include "KEPostProcessGraphView.h"

#include "PostProcess/KEPostProcessPassModel.h"
#include "PostProcess/KEPostProcessTextureModel.h"

#include "Graph/KEGraphScene.h"
#include "Graph/Node/KEGraphNodeView.h"
#include "Graph/Utility/KEGraphEmtpyNodeModel.h"
#include "Graph/Connection/KEGraphConnectionControl.h"

KEPostProcessGraphView::KEPostProcessGraphView()
{
	RegisterModel(KEPostProcessPassModel::ModelName, []()->KEGraphNodeModelPtr
	{
		return KEGraphNodeModelPtr(new KEPostProcessPassModel(GetProcessManager()->CreatePass()));
	});

	Sync();
}

KEPostProcessGraphView::~KEPostProcessGraphView()
{
}

void KEPostProcessGraphView::BuildConnection(IKPostProcessNode* node, const std::unordered_map<IKPostProcessNode*, KEGraphNodeControl*>& node2GraphNode, std::unordered_set<IKPostProcessNode*>& visitedNode)
{
	if (visitedNode.find(node) != visitedNode.end())
	{
		return;
	}
	visitedNode.insert(node);

	auto it = node2GraphNode.find(node);
	assert(it != node2GraphNode.end());

	KEGraphNodeControl* outNode = it->second;

	for (int16_t i = 0; i < PostProcessPort::MAX_OUTPUT_SLOT_COUNT; ++i)
	{
		std::unordered_set<IKPostProcessConnection*> outConns;
		node->GetOutputConnection(outConns, i);
		for (IKPostProcessConnection* conn : outConns)
		{
			IKPostProcessNode* inPortNode = conn->GetInputPortNode();
			auto itIn = node2GraphNode.find(inPortNode);
			assert(itIn != node2GraphNode.end());

			KEGraphNodeControl* inNode = itIn->second;
			assert(conn->GetOutputSlot() == i);

			m_Scene->CreateConnection(inNode, conn->GetInputSlot(), outNode, conn->GetOutputSlot());

			BuildConnection(inPortNode, node2GraphNode, visitedNode);
		}
	}
}

static const float NODE_VERTICAL_GRAP = 32.0f;
static const float NODE_HORIZONTAL_GRAP = 64.0f;

bool KEPostProcessGraphView::PopulateUniqueChild(KEGraphNodeControl* node,
	std::unordered_set<KEGraphNodeControl*>& visitedNode,
	std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds)
{
	if (visitedNode.find(node) != visitedNode.end())
	{
		return false;
	}
	visitedNode.insert(node);

	std::vector<KEGraphNodeControl*> children;

	auto& state = node->GetNodeState();
	auto& geometry = node->GetGeometry();
	const auto& entries = state.GetEntries(PT_OUT);

	for (size_t i = 0, count = entries.size(); i < count; ++i)
	{
		const auto& connSet = entries[i];
		for (auto& pair : connSet)
		{
			KEGraphNodeControl* outNode = pair.second->Node(PT_IN);
			if (PopulateUniqueChild(outNode, visitedNode, uniqueChilds))
			{
				children.push_back(outNode);
			}
		}
	}

	uniqueChilds[node] = std::move(children);
	return true;
}

float KEPostProcessGraphView::CalcOutputHeight(KEGraphNodeControl* node,
	std::unordered_map<KEGraphNodeControl*, float>& records,
	const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds)
{
	if (records.find(node) != records.end())
	{
		return 0;
	}

	auto& state = node->GetNodeState();
	auto& geometry = node->GetGeometry();
	const auto& children = uniqueChilds.at(node);

	float height = 0;
	for (KEGraphNodeControl* outNode : children)
	{
		float childHeight = CalcOutputHeight(outNode, records, uniqueChilds);
		height += childHeight + NODE_VERTICAL_GRAP;
	}
	if (children.size() > 0)
	{
		height -= NODE_VERTICAL_GRAP;
	}
	height = std::max(height, (float)geometry.BoundingRect().height());
	records[node] = height;
	return height;
}

void KEPostProcessGraphView::PlaceNodeX(KEGraphNodeControl* node,
	const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds)
{
	auto view = node->GetView();
	auto& state = node->GetNodeState();
	
	const auto& children = uniqueChilds.at(node);
	for (KEGraphNodeControl* child : children)
	{
		auto childView = child->GetView();
		childView->setX(view->x()
			+ view->boundingRect().width() + NODE_HORIZONTAL_GRAP
			- view->boundingRect().left()
		);
		PlaceNodeX(child, uniqueChilds);
		childView->MoveConnections();
	}
}

void KEPostProcessGraphView::PlaceNodeY(KEGraphNodeControl* node,
	const std::unordered_map<KEGraphNodeControl*, float>& records,
	const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds)
{
	auto view = node->GetView();
	auto& state = node->GetNodeState();

	auto bounding = view->sceneBoundingRect();
	auto center = view->sceneBoundingRect().center();
	const auto& children = uniqueChilds.at(node);

	float nodeHeightTotal = records.at(node);
	float childHeightSum = 0;
	for (KEGraphNodeControl* child : children)
	{
		float childHeight = records.at(child);
		auto childView = child->GetView();
		// 只有一个子节点
		if (children.size() == 1)
		{
			childView->setY(view->y());
		}
		else
		{
			childView->setY(center.y() - nodeHeightTotal * 0.5f
				+ childHeightSum + childHeight * 0.5f
				- childView->boundingRect().height() * 0.5f
				- childView->boundingRect().top()
			);
		}
		childHeightSum += childHeight + NODE_VERTICAL_GRAP;
	}
	if (children.size() > 0)
	{
		childHeightSum -= NODE_VERTICAL_GRAP;
	}
	assert(childHeightSum <= nodeHeightTotal);

	for (KEGraphNodeControl* child : children)
	{
		auto childView = child->GetView();
		PlaceNodeY(child, records, uniqueChilds);
		childView->MoveConnections();
	}
}

void KEPostProcessGraphView::PlaceNode(KEGraphNodeControl* node,
	const std::unordered_map<KEGraphNodeControl*, float>& records,
	const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds)
{
	PlaceNodeX(node, uniqueChilds);
	PlaceNodeY(node, records, uniqueChilds);
}

void KEPostProcessGraphView::CollectNoneInputNode(std::unordered_set<KEGraphNodeControl*>& noneInputNodes)
{
	const auto& nodeDicts = m_Scene->GetAllNodes();
	for (const auto& pair : nodeDicts)
	{
		KEGraphNodeControl* node = pair.second.get();
		auto state = node->GetNodeState();
		auto entries = state.GetEntries(PT_IN);
		bool empty = true;

		for (size_t i = 0, count = entries.size(); i < count; ++i)
		{
			if (entries[i].size() > 0)
			{
				empty = false;
				break;
			}
		}

		if (empty)
		{
			noneInputNodes.insert(node);
		}
	}
}

bool KEPostProcessGraphView::Sync()
{
	m_Scene->ClearScene();

	IKPostProcessManager* mgr = GetProcessManager();
	IKPostProcessPass* startPass = mgr->GetStartPointPass()->CastPass();

	KPostProcessNodeSet allNode;
	mgr->GetAllNodes(allNode);

	auto centerPos = mapToScene(width() / 2, height() / 2);

	std::unordered_map<IKPostProcessNode*, KEGraphNodeControl*> node2GraphNode;
	for (IKPostProcessNodePtr node : allNode)
	{
		PostProcessNodeType nodeType = node->GetType();

		if(nodeType == PPNT_PASS)
		{
			IKPostProcessPass* pass = node->CastPass();
			KEGraphNodeModelPtr model = KEGraphNodeModelPtr(new KEPostProcessPassModel(node));
			KEGraphNodeControl* graphNode = m_Scene->CreateNode(std::move(model));
			graphNode->GetView()->setPos(centerPos);
			node2GraphNode[node.get()] = graphNode;
		}
		else
		{
			// TODO
		}
	}

	std::unordered_set<IKPostProcessNode*> visitedNode;
	BuildConnection(startPass, node2GraphNode, visitedNode);

	AutoLayout();

	return true;
}

QRectF KEPostProcessGraphView::AllNodeBoundingRect() const
{
	QRectF rect;
	const auto& nodeDicts = m_Scene->GetAllNodes();
	for (const auto& pair : nodeDicts)
	{
		KEGraphNodeControl* node = pair.second.get();
		QRectF nodeRect = node->GetView()->sceneBoundingRect();
		rect = rect.united(nodeRect);
	}
	return rect;
}

bool KEPostProcessGraphView::AutoLayout()
{
	std::unordered_set<KEGraphNodeControl*> noneInputNodes;
	// 1.收集入度为0的节点
	{
		CollectNoneInputNode(noneInputNodes);
	}
	// 1.5 如果找不到入度为0的节点 直接返回
	if (noneInputNodes.empty())
	{
		return true;
	}

	// 2.收集在排版后每个节点的子节点
	std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>> uniqueChilds;
	{
		std::unordered_set<KEGraphNodeControl*> visitNodes;
		for (KEGraphNodeControl* node : noneInputNodes)
		{
			PopulateUniqueChild(node, visitNodes, uniqueChilds);
		}
	}

	// 3.添加一个虚拟根节点 连接到所有入度为0的节点上
	KEGraphNodeControl* startNode = nullptr;
	{
		auto centerPos = mapToScene(width() / 2, height() / 2);
		KEGraphNodeModelPtr model = KEGraphNodeModelPtr(new KEGraphEmtpyNodeModel(0, 1, false));
		startNode = m_Scene->CreateNode(std::move(model));
		startNode->GetView()->setPos(centerPos);
		for (KEGraphNodeControl* outNode : noneInputNodes)
		{
			uniqueChilds[startNode].push_back(outNode);
		}
	}
	// 4.排列节点
	{
		std::unordered_map<KEGraphNodeControl*, float> nodeChildHeight;
		CalcOutputHeight(startNode, nodeChildHeight, uniqueChilds);
		PlaceNode(startNode, nodeChildHeight, uniqueChilds);
	}

	// 5.移除虚拟根节点
	m_Scene->RemoveNode(startNode->ID());

	// 6.调整view位置与缩放
	{
		QRectF sceneBounding = AllNodeBoundingRect();
		QPointF viewTopLeft = mapToScene(QPoint(0, 0));
		QPointF viewBottomRight = mapToScene(QPoint(width(), height()));

		float width = viewBottomRight.x() - viewTopLeft.x();
		float height = viewBottomRight.y() - viewTopLeft.y();

		float factor = std::min(width / sceneBounding.width(), height / sceneBounding.height());
		scale(factor, factor);

		setSceneRect(sceneBounding);
	}

	return true;
}

bool KEPostProcessGraphView::Construct()
{
	return GetProcessManager()->Construct();
}