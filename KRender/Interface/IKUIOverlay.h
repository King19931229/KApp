#pragma once
#include "Interface/IKRenderConfig.h"
#include "KBase/Publish/KInput.h"

enum UIOverlayColor
{
	UI_COLOR_TEXT,
	UI_COLOR_WINDOW_BACKGROUND,
	UI_COLOR_MENUBAR_BACKGROUND
};

struct KUIColor
{
	float r, g, b, a;
};

struct IKUIOverlay
{
	virtual ~IKUIOverlay() {}

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
	virtual bool Resize(size_t width, size_t height) = 0;
	virtual bool Update() = 0;
	virtual bool Draw(IKRenderPassPtr renderPass, class KRHICommandList& commandList) = 0;

	virtual bool SetMousePosition(unsigned int x, unsigned int y) = 0;
	virtual bool SetMouseDown(InputMouseButton button, bool down) = 0;
	virtual bool SetMouseScroll(float x, float y) = 0;

	virtual bool StartNewFrame() = 0;
	virtual bool EndNewFrame() = 0;
};