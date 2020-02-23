#pragma once
#include <memory>
#include <functional>

enum PortType
{
	PT_NONE,
	PT_IN,
	PT_OUT
};

typedef int32_t PortIndexType;
enum PortIndex : PortIndexType
{
	INVALID_PORT_INDEX = -1
};

enum ConnectionPolicy
{
	CP_ONE,
	CP_MANY
};

class KEGraphScene;
typedef std::unique_ptr<KEGraphScene> KEGraphScenePtr;

class KEGraphNodeView;
typedef std::unique_ptr<KEGraphNodeView> KEGraphNodeViewPtr;

class KEGraphNodeControl;
typedef std::shared_ptr<KEGraphNodeControl> KEGraphNodeControlPtr;

class KEGraphNodeModel;
typedef std::unique_ptr<KEGraphNodeModel> KEGraphNodeModelPtr;

class KEGraphNodeData;
typedef std::shared_ptr<KEGraphNodeData> KEGraphNodeDataPtr;

class KEGraphConnectionView;
typedef std::unique_ptr<KEGraphConnectionView> KEGraphConnectionViewPtr;

class KEGraphConnectionControl;
typedef std::unique_ptr<KEGraphConnectionControl> KEGraphConnectionControlPtr;

class KEGraphRegistrar;
typedef std::unique_ptr<KEGraphRegistrar> KEGraphRegistrarPtr;

typedef std::function<KEGraphNodeModelPtr(void)> KEGraphNodeModelCreateFunc;

typedef std::function<KEGraphNodeDataPtr(KEGraphNodeDataPtr)> GraphNodeDataConverterFunc;