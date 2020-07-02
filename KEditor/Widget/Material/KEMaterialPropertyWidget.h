#pragma once
#include <QMainWindow>
#include "KRender/Interface/IKMaterial.h"

class KEMaterialPropertyWidget : public QMainWindow
{
	Q_OBJECT
protected:
	QWidget* m_MaterialWindow;
public:
	KEMaterialPropertyWidget(QWidget *parent = Q_NULLPTR);
	~KEMaterialPropertyWidget();

	QSize sizeHint() const override;

	bool Init(IKMaterial* material);
	bool UnInit();
};