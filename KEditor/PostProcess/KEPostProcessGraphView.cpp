#include "KEPostProcessGraphView.h"

#include "PostProcess/KEPostProcessPassModel.h"
#include "Graph/KEGraphScene.h"
#include "Graph/Node/KEGraphNodeView.h"
#include "Graph/Utility/KEGraphEmtpyNodeModel.h"
#include "Graph/Connection/KEGraphConnectionControl.h"

KEPostProcessGraphView::KEPostProcessGraphView()
{
}

KEPostProcessGraphView::~KEPostProcessGraphView()
{
}

void KEPostProcessGraphView::BuildConnection(IKPostProcessPass* pass, const std::unordered_map<IKPostProcessPass*, KEGraphNodeControl*>& pass2node, std::unordered_set<IKPostProcessPass*>& visitedPass)
{
	if (visitedPass.find(pass) != visitedPass.end())
	{
		return;
	}
	visitedPass.insert(pass);

	auto it = pass2node.find(pass);
	assert(it != pass2node.end());

	KEGraphNodeControl* outNode = it->second;

	for (int16_t i = 0; i < MAX_OUTPUT_SLOT_COUNT; ++i)
	{
		KPostProcessConnectionSet outConns;
		pass->GetOutputConnection(outConns, i);
		for (IKPostProcessConnection* conn : outConns)
		{
			IKPostProcessPass* inPass = conn->GetInputPortPass();
			auto itIn = pass2node.find(inPass);
			assert(itIn != pass2node.end());

			KEGraphNodeControl* inNode = itIn->second;
			assert(conn->GetOutputSlot() == i);

			m_Scene->CreateConnection(inNode, conn->GetInputSlot(), outNode, conn->GetOutputSlot());

			BuildConnection(inPass, pass2node, visitedPass);
		}
	}
}

static const float NODE_VERTICAL_GRAP = 5.0f;
static const float NODE_HORIZONTAL_GRAP = 35.0f;


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
			KEGraphNodeControl* outNode = pair.second->GetNode(PT_IN);
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
		childView->setX(view->x() + view->boundingRect().width() + NODE_HORIZONTAL_GRAP);
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
				- childView->boundingRect().height() * 0.5f);
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
	IKPostProcessPass* startPass = mgr->GetStartPointPass();

	KPostProcessPassSet allPass;
	mgr->GetAllPasses(allPass);

	auto centerPos = mapToScene(width() / 2, height() / 2);

	std::unordered_map<IKPostProcessPass*, KEGraphNodeControl*> pass2node;
	for (IKPostProcessPass* pass : allPass)
	{
		KEGraphNodeModelPtr model = KEGraphNodeModelPtr(new KEPostProcessPassModel(pass));
		KEGraphNodeControl* node = m_Scene->CreateNode(std::move(model));
		node->GetView()->setPos(centerPos);
		pass2node[pass] = node;
	}

	allPass.clear();
	BuildConnection(startPass, pass2node, allPass);

	AutoLayout();

	return true;
}

bool KEPostProcessGraphView::AutoLayout()
{
	std::unordered_set<KEGraphNodeControl*> noneInputNodes;

	{
		CollectNoneInputNode(noneInputNodes);
	}

	std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>> uniqueChilds;
	{
		std::unordered_set<KEGraphNodeControl*> visitNodes;
		for (KEGraphNodeControl* node : noneInputNodes)
		{
			PopulateUniqueChild(node, visitNodes, uniqueChilds);
		}
	}

	KEGraphNodeControl* startNode = nullptr;
	{
		auto centerPos = mapToScene(width() / 2, height() / 2);
		KEGraphNodeModelPtr model = KEGraphNodeModelPtr(new KEGraphEmtpyNodeModel(0, 1));
		startNode = m_Scene->CreateNode(std::move(model));
		startNode->GetView()->setPos(centerPos);
		for (KEGraphNodeControl* outNode : noneInputNodes)
		{
			uniqueChilds[startNode].push_back(outNode);
		}
	}

	{
		std::unordered_map<KEGraphNodeControl*, float> nodeChildHeight;
		CalcOutputHeight(startNode, nodeChildHeight, uniqueChilds);
		PlaceNode(startNode, nodeChildHeight, uniqueChilds);
	}

	m_Scene->RemoveNode(startNode);

	return true;
}

bool KEPostProcessGraphView::Apply()
{
	return false;
}