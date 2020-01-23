#pragma once
#include <QGraphicsScene>

class KEGraphScene : public QGraphicsScene
{
	Q_OBJECT
public:
	KEGraphScene(QObject * parent);
	~KEGraphScene();
};