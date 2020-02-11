#pragma once
#include "Graph/KEGraphView.h"

#include "KRender/Interface/IKPostProcess.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

class KEPostProcessGraphView : public KEGraphView
{
protected:
	void BuildConnection(IKPostProcessNode* node, const std::unordered_map<IKPostProcessNode*, KEGraphNodeControl*>& node2GraphNode, std::unordered_set<IKPostProcessNode*>& visitedNode);

	bool PopulateUniqueChild(KEGraphNodeControl* node,
		std::unordered_set<KEGraphNodeControl*>& visitedNode,
		std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds);

	float CalcOutputHeight(KEGraphNodeControl* node,
		std::unordered_map<KEGraphNodeControl*, float>& records,
		const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds);

	void PlaceNodeX(KEGraphNodeControl* node,
		const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds);

	void PlaceNodeY(KEGraphNodeControl* node,
		const std::unordered_map<KEGraphNodeControl*, float>& records,
		const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds);

	void PlaceNode(KEGraphNodeControl* node,
		const std::unordered_map<KEGraphNodeControl*, float>& records,
		const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds);

	void CollectNoneInputNode(std::unordered_set<KEGraphNodeControl*>& noneInputNodes);

	QRectF AllNodeBoundingRect() const;
public:
	KEPostProcessGraphView();
	virtual ~KEPostProcessGraphView();

	bool Sync();
	bool AutoLayout();
	bool Apply();
};