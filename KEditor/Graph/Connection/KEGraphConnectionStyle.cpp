#include "KEGraphConnectionStyle.h"

QColor KEGraphConnectionStyle::ConstructionColor = QColor("gray");
QColor KEGraphConnectionStyle::SelectedColor = QColor("gray");
QColor KEGraphConnectionStyle::SelectedHaloColor = QColor("deepskyblue");
QColor KEGraphConnectionStyle::HoveredColor = QColor("deepskyblue");

float KEGraphConnectionStyle::LineWidth = 3.0f;
float KEGraphConnectionStyle::ConstructionLineWidth = 2.0f;
float KEGraphConnectionStyle::PointDiameter = 10.0f;

bool KEGraphConnectionStyle::UseDataDefinedColors = false;

QColor KEGraphConnectionStyle::NormalColor(QString typeId)
{
	std::size_t hash = qHash(typeId);
	std::size_t const hue_range = 0xFF;
	qsrand(hash);
	std::size_t hue = qrand() % hue_range;
	std::size_t sat = 120 + hash % 129;
	return QColor::fromHsl(hue, sat, 160);
}

QColor KEGraphConnectionStyle::NormalColor()
{
	return QColor("black");
}