#pragma once
#include "Interface/IKRenderConfig.h"

struct IKRenderWindow
{
	virtual ~IKRenderWindow() {}

	virtual bool Init(size_t top, size_t left, size_t width, size_t height, bool resizable) = 0;
	virtual bool UnInit() = 0;

	virtual bool Loop() = 0;

	virtual bool GetPosition(size_t &top, size_t &left) = 0;
	virtual bool SetPosition(size_t top, size_t left) = 0;

	virtual bool GetSize(size_t &width, size_t &height) = 0;
	virtual bool SetSize(size_t width, size_t height) = 0;

	virtual bool SetResizable(bool resizable) = 0;
	virtual bool IsResizable() = 0;
};

EXPORT_DLL IKRenderWindowPtr CreateRenderWindow(RenderDevice platform);