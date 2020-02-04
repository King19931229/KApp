#pragma once
#include "Graph/KEGraphView.h"

#include "KRender/Interface/IKPostProcess.h"
#include <unordered_map>
#include <unordered_set>

class KEPostProcessGraphView : public KEGraphView
{
protected:
	void CollectPass(IKPostProcessPass* pass, std::unordered_set<IKPostProcessPass*>& allPass);
	float CalcOutputHeight(KEGraphNodeControl* node, std::unordered_map<KEGraphNodeControl*, float>& records);
	void BuildConnection(IKPostProcessPass* pass, const std::unordered_map<IKPostProcessPass*, KEGraphNodeControl*>& pass2node, std::unordered_set<IKPostProcessPass*>& visitedPass);
	void PlaceNode(KEGraphNodeControl* node, std::unordered_map<KEGraphNodeControl*, float>& records);
public:
	KEPostProcessGraphView();
	virtual ~KEPostProcessGraphView();

	bool Sync();
	bool Apply();
};