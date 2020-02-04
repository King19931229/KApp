#include "KEPostProcessGraphView.h"

#include "PostProcess/KEPostProcessPassModel.h"
#include "Graph/KEGraphScene.h"
#include "Graph/Node/KEGraphNodeView.h"
#include "Graph/Connection/KEGraphConnectionControl.h"

KEPostProcessGraphView::KEPostProcessGraphView()
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

float KEPostProcessGraphView::CalcOutputHeight(KEGraphNodeControl* node, std::unordered_map<KEGraphNodeControl*, float>& records)
{
	auto it = records.find(node);
	if (it != records.end())
	{
		return it->second;
	}

	float height = 0;
	float maxChildHeight = 0;
	std::unordered_set<KEGraphNodeControl*> uniqueChild;
	auto& state = node->GetNodeState();
	auto& geometry = node->GetGeometry();

	for (int16_t i = 0; i < MAX_OUTPUT_SLOT_COUNT; ++i)
	{
		auto connSet = state.Connections(PT_OUT, i);
		for (auto& pair : connSet)
		{
			KEGraphNodeControl* outNode = pair.second->GetNode(PT_IN);
			uniqueChild.insert(outNode);
			maxChildHeight = std::max(maxChildHeight, CalcOutputHeight(outNode, records));
		}
	}

	if (uniqueChild.size() > 0)
	{
		height = (float)(uniqueChild.size() - 1) * NODE_VERTICAL_GRAP + (float)uniqueChild.size() * maxChildHeight;
	}

	height = std::max(height, (float)geometry.BoundingRect().height());

	records[node] = height;
	return height;
}

void KEPostProcessGraphView::PlaceNode(KEGraphNodeControl* node, std::unordered_map<KEGraphNodeControl*, float>& records)
{
	std::unordered_set<KEGraphNodeControl*> uniqueChild;

	auto view = node->GetView();
	auto& state = node->GetNodeState();
	auto& geometry = node->GetGeometry();

	for (int16_t i = 0; i < MAX_OUTPUT_SLOT_COUNT; ++i)
	{
		auto connSet = state.Connections(PT_OUT, i);
		for (auto& pair : connSet)
		{
			KEGraphNodeControl* outNode = pair.second->GetNode(PT_IN);
			uniqueChild.insert(outNode);
		}
	}

	auto center = view->sceneBoundingRect().center();

	float childHeight = records[node];
	float segmentY = center.y() - childHeight * 0.5f;
	float segmentHeight = childHeight / (float)uniqueChild.size();

	size_t segmentCounter = 0;

	for (KEGraphNodeControl* child : uniqueChild)
	{
		auto childView = child->GetView();

		childView->setX(view->x() + view->boundingRect().width() + NODE_HORIZONTAL_GRAP);
		// 只有一个子节点
		if (segmentHeight == childHeight)
		{
			childView->setY(view->y());
		}
		else
		{
			childView->setY(segmentY + segmentHeight * segmentCounter + segmentHeight * 0.5f);
		}
		++segmentCounter;

		PlaceNode(child, records);
	}
	view->MoveConnections();
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

	KEGraphNodeControl* startNode = pass2node[startPass];

	std::unordered_map<KEGraphNodeControl*, float> nodeChildHeight;
	CalcOutputHeight(startNode, nodeChildHeight);
	PlaceNode(startNode, nodeChildHeight);

	return true;
}

bool KEPostProcessGraphView::Apply()
{
	return false;
}