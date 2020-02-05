#include "KEPostProcessGraphView.h"

#include "PostProcess/KEPostProcessPassModel.h"
#include "Graph/KEGraphScene.h"
#include "Graph/Node/KEGraphNodeView.h"
#include "Graph/Connection/KEGraphConnectionControl.h"

KEPostProcessGraphView::KEPostProcessGraphView()
	: m_StartNode(nullptr)
{
}

KEPostProcessGraphView::~KEPostProcessGraphView()
{
}

void KEPostProcessGraphView::CollectPass(IKPostProcessPass* pass, std::unordered_set<IKPostProcessPass*>& allPass)
{
	if (allPass.find(pass) == allPass.end())
	{
		allPass.insert(pass);
		for (int16_t i = 0; i < MAX_OUTPUT_SLOT_COUNT; ++i)
		{
			KPostProcessConnectionSet outConns;
			pass->GetOutputConnection(outConns, i);
			for (IKPostProcessConnection* conn : outConns)
			{
				IKPostProcessPass* inPass = conn->GetInputPortPass();
				CollectPass(inPass, allPass);
			}
		}
	}
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

float KEPostProcessGraphView::CalcOutputHeight(KEGraphNodeControl* node,
	std::unordered_map<KEGraphNodeControl*, float>& records,
	std::unordered_map<KEGraphNodeControl* ,std::vector<KEGraphNodeControl*>>& uniqueChilds)
{
	auto it = records.find(node);
	if (it != records.end())
	{
		return 0;
	}
	records[node] = 0;

	float height = 0;

	std::vector<KEGraphNodeControl*> uniqueChild;

	auto& state = node->GetNodeState();
	auto& geometry = node->GetGeometry();

	for (int16_t i = 0; i < MAX_OUTPUT_SLOT_COUNT; ++i)
	{
		auto connSet = state.Connections(PT_OUT, i);
		for (auto& pair : connSet)
		{
			KEGraphNodeControl* outNode = pair.second->GetNode(PT_IN);			
			float childHeight = CalcOutputHeight(outNode, records, uniqueChilds);
			if (childHeight > 0)
			{
				uniqueChild.push_back(outNode);
				height += childHeight + NODE_VERTICAL_GRAP;
			}
		}
	}
	height -= NODE_VERTICAL_GRAP;	

	height = std::max(height, (float)geometry.BoundingRect().height());

	records[node] = height;
	uniqueChilds[node] = std::move(uniqueChild);

	return height;
}

void KEPostProcessGraphView::PlaceNodeX(KEGraphNodeControl* node,
	const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds)
{
	auto view = node->GetView();
	auto& state = node->GetNodeState();

	for (int16_t i = 0; i < MAX_OUTPUT_SLOT_COUNT; ++i)
	{
		auto connSet = state.Connections(PT_OUT, i);
		for (auto& pair : connSet)
		{
			const auto& children = uniqueChilds.at(node);
			for (KEGraphNodeControl* child : children)
			{
				auto childView = child->GetView();
				childView->setX(view->x() + view->boundingRect().width() + NODE_HORIZONTAL_GRAP);
				PlaceNodeX(child, uniqueChilds);
				childView->MoveConnections();
			}
		}
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
	childHeightSum -= NODE_VERTICAL_GRAP;
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

bool KEPostProcessGraphView::Sync()
{
	m_Scene->ClearScene();

	IKPostProcessManager* mgr = GetProcessManager();
	IKPostProcessPass* startPass = mgr->GetStartPointPass();

	std::unordered_set<IKPostProcessPass*> allPass;
	CollectPass(startPass, allPass);

	auto centerPos = mapToScene(width() / 2, height() / 2);

	std::unordered_map<IKPostProcessPass*, KEGraphNodeControl*> pass2node;
	for (IKPostProcessPass* pass : allPass)
	{
		KEGraphNodeModelPtr model = KEGraphNodeModelPtr(new KEPostProcessPassModel(pass));
		KEGraphNodeControl* node = m_Scene->CreateNode(std::move(model));
		node->GetView()->setPos(centerPos);
		// TODO user data
		pass2node[pass] = node;
	}

	allPass.clear();
	BuildConnection(startPass, pass2node, allPass);

	m_StartNode = pass2node[startPass];
	
	AutoLayout();

	return true;
}

bool KEPostProcessGraphView::AutoLayout()
{
	if (m_StartNode)
	{
		std::unordered_map<KEGraphNodeControl*, float> nodeChildHeight;
		std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>> uniqueChilds;
		CalcOutputHeight(m_StartNode, nodeChildHeight, uniqueChilds);
		PlaceNode(m_StartNode, nodeChildHeight, uniqueChilds);
	}
	return true;
}

bool KEPostProcessGraphView::Apply()
{
	return false;
}