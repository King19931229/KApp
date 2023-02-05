#pragma once
#include <stdint.h>

class KRenderImGui
{
public:
	enum MenuItem
	{
		CSM,
		SSR,
		DOF,
		RTAO,
		SVO_GI,
		CLIPMAP_GI,
		HIZ_OC,
		HARDWARE_OC,
		VOLUMETIRIC_FOG,
		MENU_ITEM_COUNT
	};
	static const char* MenuName[MENU_ITEM_COUNT];
	static constexpr char* ConfigFile = "render_gui.json";
protected:
	bool m_MenuEnable[MENU_ITEM_COUNT];	
public:
	KRenderImGui();
	~KRenderImGui();

	void Open();
	void Exit();
	void Run();

	bool WantCaptureInput() const;
};

extern KRenderImGui GRenderImGui;