#include "KEGraphNodeStyle.h"

QColor KEGraphNodeStyle::NormalBoundaryColor = QColor(255, 255, 255);
QColor KEGraphNodeStyle::SelectedBoundaryColor = QColor(255, 165, 0);
QColor KEGraphNodeStyle::GradientColor0 = QColor("gray");
QColor KEGraphNodeStyle::GradientColor1 = QColor(80, 80, 80);
QColor KEGraphNodeStyle::GradientColor2 = QColor(64, 64, 64);
QColor KEGraphNodeStyle::GradientColor3 = QColor(58, 58, 58);
QColor KEGraphNodeStyle::ShadowColor = QColor(20, 20, 20);
QColor KEGraphNodeStyle::FontColor = QColor("white");
QColor KEGraphNodeStyle::FontColorFaded = QColor("gray");

QColor KEGraphNodeStyle::ConnectionPointColor = QColor(169, 169, 169);
QColor KEGraphNodeStyle::FilledConnectionPointColor = QColor("cyan");

QColor KEGraphNodeStyle::WarningColor = QColor("red");
QColor KEGraphNodeStyle::ErrorColor = QColor(128, 128, 0);

float KEGraphNodeStyle::PenWidth = 1.0f;
float KEGraphNodeStyle::HoveredPenWidth = 1.5f;

float KEGraphNodeStyle::ConnectionPointDiameter = 8.0f;

float KEGraphNodeStyle::Opacity = 0.8f;