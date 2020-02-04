#pragma once
#include "KEGraphWidget.h"

class KEPostProcessGraphWidget : public KEGraphWidget
{
public:
	KEPostProcessGraphWidget();
	~KEPostProcessGraphWidget();

	KEGraphView* CreateViewImpl() override;
	bool Init() override;
	bool UnInit() override;

public Q_SLOTS:
	void Sync();
	void AutoLayout();
};