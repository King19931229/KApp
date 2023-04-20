#pragma once
#include <stdint.h>

class KRenderImGui
{
public:
	enum SettingMenuItem
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
		SETTING_MENU_ITEM_COUNT
	};
	static const char* SettingMenuName[SETTING_MENU_ITEM_COUNT];

	enum DebugMenuItem
	{
		DEFERRED,
		ADVANCED_CONTROL,
		VULKAN,
		DEBUG_MENU_ITEM_COUNT
	};
	static const char* DebugMenuName[DEBUG_MENU_ITEM_COUNT];

	static constexpr char* ConfigFile = "render_gui.json";
protected:
	bool m_SettingMenuEnable[SETTING_MENU_ITEM_COUNT];
	bool m_DebugMenuEnable[DEBUG_MENU_ITEM_COUNT];
public:
	KRenderImGui();
	~KRenderImGui();

	void Open();
	void Exit();
	void Run();

	bool WantCaptureInput() const;
};

extern KRenderImGui GRenderImGui;