#pragma once
#include "KEGraphWidget.h"

class KEPostProcessGraphWidget : public KEGraphWidget
{
public:
	KEPostProcessGraphWidget();
	~KEPostProcessGraphWidget();

public Q_SLOTS:
	void SyncPostprocess();
};