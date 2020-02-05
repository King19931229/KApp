#pragma once
#include "Graph/KEGraphView.h"

#include "KRender/Interface/IKPostProcess.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

class KEPostProcessGraphView : public KEGraphView
{
protected:
	KEGraphNodeControl* m_StartNode;
	void CollectPass(IKPostProcessPass* pass, std::unordered_set<IKPostProcessPass*>& allPass);

	float CalcOutputHeight(KEGraphNodeControl* node,
		std::unordered_map<KEGraphNodeControl*, float>& records,
		std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds);

	void BuildConnection(IKPostProcessPass* pass, const std::unordered_map<IKPostProcessPass*, KEGraphNodeControl*>& pass2node, std::unordered_set<IKPostProcessPass*>& visitedPass);

	void PlaceNodeX(KEGraphNodeControl* node,
		const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds);

	void PlaceNodeY(KEGraphNodeControl* node,
		const std::unordered_map<KEGraphNodeControl*, float>& records,
		const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds);

	void PlaceNode(KEGraphNodeControl* node,
		const std::unordered_map<KEGraphNodeControl*, float>& records,
		const std::unordered_map<KEGraphNodeControl*, std::vector<KEGraphNodeControl*>>& uniqueChilds);
public:
	KEPostProcessGraphView();
	virtual ~KEPostProcessGraphView();

	bool Sync();
	bool AutoLayout();
	bool Apply();
};