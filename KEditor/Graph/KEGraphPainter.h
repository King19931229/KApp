#pragma once
#include <QtGui/QPainter>
#include "Node/KEGraphNodeControl.h"
#include "Node/KEGraphNodeModel.h"
#include "Node/KEGraphNodeView.h"
#include "Node/KEGraphNodeGeometry.h"
#include "Node/KEGraphNodeState.h"
#include "KEGraphScene.h"

class KEGraphPainter
{
public:
	static void	Paint(QPainter* painter, KEGraphNodeControl& node, KEGraphScene const& scene);
	static void	DrawNodeRect(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeModel const* model, KEGraphNodeView	 const & graphicsObject);
	static void	DrawModelName(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model);
	static void DrawEntryLabels(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model);
	static void	DrawConnectionPoints(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model, KEGraphScene const & scene);
	static void	DrawFilledConnectionPoints(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model);
	static void	DrawResizeRect(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeModel const * model);
	static void	DrawValidationRect(QPainter * painter, KEGraphNodeGeometry& geom, KEGraphNodeModel const * model, KEGraphNodeView const & graphicsObject);
};