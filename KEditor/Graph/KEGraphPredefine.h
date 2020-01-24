#pragma once
#include <memory>

class KEGraphScene;
typedef std::unique_ptr<KEGraphScene> KEGraphScenePtr;

class KEGraphNodeView;
typedef std::unique_ptr<KEGraphNodeView> KEGraphNodeViewPtr;

class KEGraphNodeControl;
typedef std::unique_ptr<KEGraphNodeControl> KEGraphNodeControlPtr;

class KEGraphNodeModel;
typedef std::unique_ptr<KEGraphNodeModel> KEGraphNodeModelPtr;

enum PortType
{
	PT_NONE,
	PT_IN,
	PT_OUT
};