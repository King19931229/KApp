#pragma once
#include <QColor>

class KEGraphConnectionStyle
{
public:
	static QColor ConstructionColor;
	static QColor SelectedColor;
	static QColor SelectedHaloColor;
	static QColor HoveredColor;

	static float LineWidth;
	static float ConstructionLineWidth;
	static float PointDiameter;

	static bool UseDataDefinedColors;

	static QColor NormalColor(QString typeId);
	static QColor NormalColor();
};