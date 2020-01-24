#pragma once
#include <QtWidgets/QWidget>
#include "Graph/KEGraphPredefine.h"

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
	virtual QString	PortCaption(PortType type, uint16_t index) const { return QString(); }
	/// It is possible to hide port caption in GUI
	virtual bool PortCaptionVisible(PortType type, uint16_t index) const { return false; }
	/// Name makes this model unique
	virtual QString	Name() const = 0;
};