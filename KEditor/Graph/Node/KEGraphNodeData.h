#pragma once
#include <QtCore/QString>

struct KEGraphNodeDataType
{
	QString id;
	QString name;
};

class KEGraphNodeData
{
public:

	virtual ~KEGraphNodeData() {}

	virtual bool SameType(KEGraphNodeData const &nodeData) const
	{
		return (this->Type().id == nodeData.Type().id);
	}

	/// Type for inner use
	virtual KEGraphNodeDataType Type() const = 0;
};