#pragma once
#include "KEGraphWidget.h"

class KEPostProcessGraphWidget : public KEGraphWidget
{
protected:
public:
	KEPostProcessGraphWidget();
	~KEPostProcessGraphWidget();

	KEGraphView* CreateViewImpl() override;
	bool Init() override;
	bool UnInit() override;
};