#pragma once
#include <QColor>

class KEGraphNodeStyle
{
public:
	static QColor NormalBoundaryColor;
	static QColor SelectedBoundaryColor;
	static QColor GradientColor0;
	static QColor GradientColor1;
	static QColor GradientColor2;
	static QColor GradientColor3;
	static QColor ShadowColor;
	static QColor FontColor;
	static QColor FontColorFaded;

	static QColor ConnectionPointColor;
	static QColor FilledConnectionPointColor;

	static QColor WarningColor;
	static QColor ErrorColor;

	static float PenWidth;
	static float HoveredPenWidth;

	static float ConnectionPointDiameter;

	static float Opacity;
};