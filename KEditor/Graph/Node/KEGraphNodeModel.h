#pragma once
#include <QtWidgets/QWidget>

#include "Graph/KEGraphConfig.h"
#include "KEGraphNodeData.h"

class KEGraphNodeModel : public QObject
{
	Q_OBJECT
public:
	KEGraphNodeModel();
	virtual	~KEGraphNodeModel();

	/// Caption is used in GUI
	virtual QString	Caption() const = 0;
	/// It is possible to hide caption in GUI
	virtual bool CaptionVisible() const { return true; }
	/// Port caption is used in GUI to label individual ports
	virtual QString	PortCaption(PortType type, PortIndexType index) const { return QString(); }
	/// It is possible to hide port caption in GUI
	virtual bool PortCaptionVisible(PortType type, PortIndexType index) const { return false; }
	/// Name makes this model unique
	virtual QString	Name() const = 0;

	virtual	bool Resizable() const { return false; }
	virtual bool Deletable() const { return true; }
	virtual bool Redoable() const { return true; }

	virtual	unsigned int NumPorts(PortType portType) const = 0;
	virtual	KEGraphNodeDataType DataType(PortType portType, PortIndexType portIndex) const = 0;

	/// Triggers the algorithm
	virtual	void SetInData(KEGraphNodeDataPtr nodeData, PortIndexType port) = 0;
	virtual KEGraphNodeDataPtr OutData(PortIndexType port) = 0;
	virtual	QWidget* EmbeddedWidget() = 0;

	virtual	ConnectionPolicy PortOutConnectionPolicy(PortIndexType portIndex) const {	return CP_MANY; }

Q_SIGNALS:
	void SingalDataUpdated(PortIndexType index);
	void SingalDataInvalidated(PortIndexType index);
	void SingalComputingStarted();
	void SingalComputingFinished();
	void SingalEmbeddedWidgetSizeUpdated();
};