#pragma once
#include "Interface/IKRenderConfig.h"
#include "KBase/Publish/KInput.h"

struct IKUIOverlay
{
	virtual ~IKUIOverlay() {}

	virtual bool Init(IKRenderDevice* renderDevice, size_t frameInFlight) = 0;
	virtual bool UnInit() = 0;
	virtual bool Resize(size_t width, size_t height) = 0;
	virtual bool Update() = 0;
	virtual bool Draw(IKRenderPassPtr renderPass, IKCommandBufferPtr commandBufferPtr) = 0;

	virtual bool SetMousePosition(unsigned int x, unsigned int y) = 0;
	virtual bool SetMouseDown(InputMouseButton button, bool down) = 0;

	virtual bool Begin(const char* str) = 0;
	virtual bool SetWindowPos(unsigned int x, unsigned int y) = 0;
	virtual bool SetWindowSize(unsigned int width, unsigned int height) = 0;
	virtual bool PushItemWidth(float width) = 0;
	virtual bool PopItemWidth() = 0;
	virtual bool End() = 0;

	virtual bool StartNewFrame() = 0;
	virtual bool EndNewFrame() = 0;

	virtual bool Header(const char* caption) = 0;
	virtual bool CheckBox(const char* caption, bool* value) = 0;
	virtual bool InputFloat(const char* caption, float* value, float step, unsigned int precision) = 0;
	virtual bool SliderFloat(const char* caption, float* value, float min, float max) = 0;
	virtual bool SliderInt(const char* caption, int* value, int min, int max) = 0;
	virtual bool ComboBox(const char* caption, int* itemindex, const std::vector<std::string>& items) = 0;
	virtual bool Button(const char* caption) = 0;
	virtual void Text(const char* formatstr, ...) = 0;
};