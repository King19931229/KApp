#include "KEGraphConnectionControl.h"
#include "Graph/Connection/KEGraphConnectionControl.h"
#include "Graph/Connection/KEGraphConnectionView.h"
#include "Graph/KEGraphPort.h"
#include "Graph/KEGraphScene.h"

#include <assert.h>

KEGraphConnectionControl::KEGraphConnectionControl(PortType portType,
	KEGraphNodeControl* node,
	PortIndexType portIndex)
	: m_ID(QUuid::createUuid()),
	m_OutPortIndex(INVALID_PORT_INDEX),
	m_InPortIndex(INVALID_PORT_INDEX),
	m_ConnectionState()
{
	SetNodeToPort(node, portType, portIndex);
	SetRequiredPort(OppositePort(portType));
}

KEGraphConnectionControl::KEGraphConnectionControl(KEGraphNodeControl* nodeIn,
	PortIndexType portIndexIn,
	KEGraphNodeControl* nodeOut,
	PortIndexType portIndexOut,
	GraphNodeDataConverterFunc converter)
	: m_ID(QUuid::createUuid()),
	m_InNode(nodeIn),
	m_OutNode(nodeOut),
	m_OutPortIndex(portIndexOut),
	m_InPortIndex(portIndexIn),
	m_ConnectionState(),
	m_Converter(std::move(converter))
{
	SetNodeToPort(nodeIn, PT_IN, portIndexIn);
	SetNodeToPort(nodeOut, PT_OUT, portIndexOut);
}

KEGraphConnectionControl::~KEGraphConnectionControl()
{	
}

void KEGraphConnectionControl::Exit(KEGraphScene* scene)
{
	if (m_View->scene() == scene)
	{
		// 取消选中与Hover状态
		m_View->setSelected(false);
		m_ConnectionGeometry.SetHovered(false);

		RemoveFromNodes();

		PropagateEmptyData();

		if (m_InNode)
		{
			m_InNode->GetView()->update();
		}

		if (m_OutNode)
		{
			m_OutNode->GetView()->update();
		}

		scene->removeItem(m_View.get());
		assert(m_View->scene() == nullptr);

		if (Complete())
		{
			SingalConnectionMadeIncomplete(this);
		}
	}
}

void KEGraphConnectionControl::Enter(KEGraphScene* scene)
{
	Exit(scene);

	if (Complete())
	{
		m_InNode->GetNodeState().SetConnection(PT_IN, m_InPortIndex, *this);
		m_OutNode->GetNodeState().SetConnection(PT_OUT, m_OutPortIndex, *this);
	}

	if (m_OutNode)
	{
		// trigger data propagation
		m_OutNode->OnDataUpdated(m_OutPortIndex);
		scene->SingalConnectionCreated(this);
	}
	else
	{
		// Note: this connection isn't truly created yet. It's only partially created.
		// Thus, don't send the connectionCreated(...) signal.
		connect(this,
			&KEGraphConnectionControl::SingalConnectionCompleted,
			this,
			[scene, this](KEGraphConnectionControl* c)
		{
			scene->SingalConnectionCreated(c);
		});
	}

	scene->addItem(m_View.get());
}

QUuid KEGraphConnectionControl::ID() const
{
	return m_ID;
}

bool KEGraphConnectionControl::Complete() const
{
	return m_InNode != nullptr && m_OutNode != nullptr;
}

void KEGraphConnectionControl::SetRequiredPort(PortType dragging)
{
	m_ConnectionState.SetRequiredPort(dragging);

	switch (dragging)
	{
	case PT_OUT:
		m_OutNode = nullptr;
		m_OutPortIndex = INVALID_PORT_INDEX;
		break;

	case PT_IN:
		m_InNode = nullptr;
		m_InPortIndex = INVALID_PORT_INDEX;
		break;

	default:
		break;
	}
}

PortType KEGraphConnectionControl::RequiredPort() const
{
	return m_ConnectionState.RequiredPort();
}

void KEGraphConnectionControl::SetView(KEGraphConnectionViewPtr&& graphics)
{
	m_View = std::move(graphics);

	// This function is only called when the ConnectionGraphicsObject
	// is newly created. At this moment both end coordinates are (0, 0)
	// in Connection G.O. coordinates. The position of the whole
	// Connection G. O. in scene coordinate system is also (0, 0).
	// By moving the whole object to the Node Port position
	// we position both connection ends correctly.

	if (RequiredPort() != PT_NONE)
	{
		PortType attachedPort = OppositePort(RequiredPort());
		PortIndexType attachedPortIndex = GetPortIndex(attachedPort);

		KEGraphNodeControl* const& node = Node(attachedPort);

		QTransform nodeSceneTransform = node->GetView()->sceneTransform();

		QPointF pos = node->GetGeometry().PortScenePosition(attachedPortIndex,
			attachedPort,
			nodeSceneTransform);

		m_View->setPos(pos);
	}

	m_View->Move();
}

PortIndexType KEGraphConnectionControl::GetPortIndex(PortType portType) const
{
	PortIndexType result = INVALID_PORT_INDEX;
	switch (portType)
	{
	case PT_IN:
		result = m_InPortIndex;
		break;
	case PT_OUT:
		result = m_OutPortIndex;
		break;
	default:
		break;
	}
	return result;
}

void KEGraphConnectionControl::SetNodeToPort(KEGraphNodeControl* node, PortType portType, PortIndexType portIndex)
{
	bool wasIncomplete = !Complete();

	Node(portType) = node;

	if (portType == PT_OUT)
		m_OutPortIndex = portIndex;
	else
		m_InPortIndex = portIndex;

	m_ConnectionState.SetNoRequiredPort();

	SingalUpdated(this);
	if (Complete() && wasIncomplete)
	{
		SingalConnectionCompleted(this);
	}
}

void KEGraphConnectionControl::RemoveFromNodes() const
{
	if (m_InNode)
	{
		m_InNode->GetNodeState().EraseConnection(PT_IN, m_InPortIndex, ID());
	}

	if (m_OutNode)
	{
		m_OutNode->GetNodeState().EraseConnection(PT_OUT, m_OutPortIndex, ID());
	}
}

KEGraphNodeControl*& KEGraphConnectionControl::Node(PortType portType)
{
	switch (portType)
	{
	case PT_IN:
		return m_InNode;
	case PT_OUT:
		return m_OutNode;
	default:
		assert(false && "should not be reached");
		// keep the compiler happy
		static KEGraphNodeControl* null = nullptr;
		return null;
	}
}

void KEGraphConnectionControl::ClearNode(PortType portType)
{
	if (Complete())
	{
		SingalConnectionMadeIncomplete(this);
	}

	Node(portType) = nullptr;

	if (portType == PT_IN)
	{
		m_InPortIndex = INVALID_PORT_INDEX;
	}
	else
	{
		m_OutPortIndex = INVALID_PORT_INDEX;
	}
}

KEGraphNodeDataType KEGraphConnectionControl::DataType(PortType portType) const
{
	if (m_InNode && m_OutNode)
	{
		KEGraphNodeModel* model = (portType == PT_IN) ? m_InNode->GetModel() : m_OutNode->GetModel();
		PortIndexType index = (portType == PT_IN) ? m_InPortIndex : m_OutPortIndex;
		return model->DataType(portType, index);
	}
	else
	{
		KEGraphNodeControl* validNode;
		PortIndexType index = INVALID_PORT_INDEX;

		if ((validNode = m_InNode))
		{
			index = m_InPortIndex;
			portType = PT_IN;
		}
		else if ((validNode = m_OutNode))
		{
			index = m_OutPortIndex;
			portType = PT_OUT;
		}

		if (validNode)
		{
			const KEGraphNodeModel* model = validNode->GetModel();
			return model->DataType(portType, index);
		}
	}

	assert(false);
	return KEGraphNodeDataType();
}

void KEGraphConnectionControl::SetTypeConverter(GraphNodeDataConverterFunc converter)
{
	m_Converter = std::move(converter);
}

void KEGraphConnectionControl::PropagateData(KEGraphNodeDataPtr nodeData) const
{
	if (m_InNode)
	{
		if (m_Converter && nodeData)
		{
			nodeData = m_Converter(nodeData);
		}
		m_InNode->PropagateData(nodeData, m_InPortIndex);
	}
}

void KEGraphConnectionControl::PropagateEmptyData() const
{
	KEGraphNodeDataPtr emptyData = nullptr;
	PropagateData(emptyData);
}