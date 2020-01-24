#pragma once
#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QJsonObject>

#include "Graph/KEGraphPredefine.h"
#include "KEGraphNodeGeometry.h"

class KEGraphNodeControl : public QObject
{
	Q_OBJECT
protected:
	KEGraphNodeViewPtr m_View;
	KEGraphNodeModelPtr m_Model;
	KEGraphNodeGeometry m_Geometry;
	QUuid m_ID;
public:
	KEGraphNodeControl(KEGraphNodeModelPtr&& model);
	virtual ~KEGraphNodeControl();

	KEGraphNodeView* GetView() { return m_View.get(); }
	void SetView(KEGraphNodeViewPtr&& view);
	KEGraphNodeGeometry& GetGeometry() { return m_Geometry; }

	inline QUuid ID() const { return m_ID; }
};