#include "KRenderImGui.h"
#include "KRenderGlobal.h"
#include "KBase/Interface/IKJson.h"
#include "Internal/Vulkan/KVulkanGlobal.h"
#include "imgui.h"

KRenderImGui GRenderImGui;

const char* KRenderImGui::SettingMenuName[] =
{
	"CSM",
	"SSR",
	"DOF",
	"RTAO",
	"SVOGI",
	"ClipmapGI",
	"HiZOcclusion",
	"HardwareOcclusion",
	"VolumetricFog",
	"VirtualGeometry",
};

const char* KRenderImGui::DebugMenuName[] =
{
	"DeferredRenderer",
	"AdvancedContorl",
	"Vulkan"
};

KRenderImGui::KRenderImGui()
{
	memset(m_SettingMenuEnable, 0, sizeof(m_SettingMenuEnable));
	memset(m_DebugMenuEnable, 0, sizeof(m_DebugMenuEnable));
}

KRenderImGui::~KRenderImGui()
{
}

void KRenderImGui::Open()
{
	IKJsonDocumentPtr jsonDoc = GetJsonDocument();
	IKDataStreamPtr fileStream = GetDataStream(IT_FILEHANDLE);
	if (fileStream->Open(ConfigFile, IM_READ))
	{
		if (jsonDoc->ParseFromDataStream(fileStream))
		{
			for (uint32_t i = 0; i < SETTING_MENU_ITEM_COUNT; ++i)
			{
				assert(SettingMenuName[i]);
				if (jsonDoc->HasMember(SettingMenuName[i]))
				{
					m_SettingMenuEnable[i] = jsonDoc->GetMember(SettingMenuName[i])->GetBool();
				}
			}

			for (uint32_t i = 0; i < DEBUG_MENU_ITEM_COUNT; ++i)
			{
				assert(DebugMenuName[i]);
				if (jsonDoc->HasMember(DebugMenuName[i]))
				{
					m_DebugMenuEnable[i] = jsonDoc->GetMember(DebugMenuName[i])->GetBool();
				}
			}
		}
		fileStream->Close();
	}
}

void KRenderImGui::Exit()
{
	IKJsonDocumentPtr jsonDoc = GetJsonDocument();
	IKJsonValuePtr root = jsonDoc->GetRoot();
	for (uint32_t i = 0; i < SETTING_MENU_ITEM_COUNT; ++i)
	{
		root->AddMember(SettingMenuName[i], jsonDoc->CreateBool(m_SettingMenuEnable[i]));
	}
	for (uint32_t i = 0; i < DEBUG_MENU_ITEM_COUNT; ++i)
	{
		root->AddMember(DebugMenuName[i], jsonDoc->CreateBool(m_DebugMenuEnable[i]));
	}
	jsonDoc->SaveAsFile(ConfigFile);
}

bool KRenderImGui::WantCaptureInput() const
{
	const ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput;
}

void KRenderImGui::Run()
{
	ImGuiIO& io = ImGui::GetIO();

	io.FontGlobalScale = std::max(1.0f, 3.0f * (std::max(io.DisplaySize.x, io.DisplaySize.y) / 4096.0f));

	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
	{
		KRenderStatistics statistics;
		KRenderGlobal::Statistics.GetAllStatistics(statistics);

		// Stat
		{
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, 60), 0, ImVec2(0.5f, 0.0f));
			ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
			ImGui::Text("FPS [%f] FrameTime [%f]", statistics.frame.fps, statistics.frame.frametime);
			ImGui::Text("DrawCall [%d] Face [%d] [Primtives] [%d]", statistics.stage.drawcalls, statistics.stage.faces, statistics.stage.primtives);
			ImGui::End();
		}

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Settings"))
			{
				for (uint32_t i = 0; i < SETTING_MENU_ITEM_COUNT; ++i)
				{
					ImGui::MenuItem(SettingMenuName[i], nullptr, &m_SettingMenuEnable[i]);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Debug"))
			{
				for (uint32_t i = 0; i < DEBUG_MENU_ITEM_COUNT; ++i)
				{
					ImGui::MenuItem(DebugMenuName[i], nullptr, &m_DebugMenuEnable[i]);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		if (m_DebugMenuEnable[DEFERRED])
		{
			ImGui::Begin(DebugMenuName[DEFERRED], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			// https://github.com/ocornut/imgui/issues/1658
			const char* currentOptionText = GDeferredRenderDebugDescription[KRenderGlobal::DeferredRenderer.GetDebugOption()].name;
			if (ImGui::BeginCombo("Debug", currentOptionText))
			{
				for (int i = 0; i < DRD_COUNT; ++i)
				{
					bool isSelected = currentOptionText == GDeferredRenderDebugDescription[i].name;
					if (ImGui::Selectable(GDeferredRenderDebugDescription[i].name, isSelected))
					{
						currentOptionText = GDeferredRenderDebugDescription[i].name;
						KRenderGlobal::DeferredRenderer.SetDebugOption((DeferredRenderDebug)i);
					}
					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::End();
		}

		if (m_DebugMenuEnable[ADVANCED_CONTROL])
		{
			ImGui::Begin(DebugMenuName[ADVANCED_CONTROL], nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Checkbox("AsyncCompute", &KRenderGlobal::Renderer.GetEnableAsyncCompute());
			ImGui::Checkbox("MultithreadRender", &KRenderGlobal::Renderer.GetEnableMultithreadRender());
			ImGui::SliderInt("MultithreadCount", &KRenderGlobal::Renderer.GetMultithreadCount(), 1, 128);
			ImGui::End();
		}

		if (m_DebugMenuEnable[VULKAN])
		{
			ImGui::Begin(DebugMenuName[VULKAN], nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Checkbox("HashDescriptorUpdate", &KVulkanGlobal::hashDescriptorUpdate);
			ImGui::End();
		}

		if (m_SettingMenuEnable[CSM])
		{
			ImGui::Begin(SettingMenuName[CSM], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::CascadedShadowMap.GetEnable());

			ImGui::SliderFloat("DepthBias Slope[0]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(0), 0.0f, 5.0f);
			ImGui::SliderFloat("DepthBias Slope[1]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(1), 0.0f, 5.0f);
			ImGui::SliderFloat("DepthBias Slope[2]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(2), 0.0f, 5.0f);
			ImGui::SliderFloat("DepthBias Slope[3]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(3), 0.0f, 5.0f);

			// ImGui::SliderFloat("ShadowRange", &KRenderGlobal::CascadedShadowMap.GetShadowRange(), 0.1f, 5000.0f);
			// ImGui::SliderFloat("SplitLambda", &KRenderGlobal::CascadedShadowMap.GetSplitLambda(), 0.001f, 1.0f);
			// ImGui::SliderFloat("LightSize", &KRenderGlobal::CascadedShadowMap.GetLightSize(), 0.0f, 0.1f);

			ImGui::Checkbox("FixToScene", &KRenderGlobal::CascadedShadowMap.GetFixToScene());
			ImGui::Checkbox("FixTexel", &KRenderGlobal::CascadedShadowMap.GetFixTexel());
			ImGui::Checkbox("Minimize Draw", &KRenderGlobal::CascadedShadowMap.GetMinimizeShadowDraw());

			ImGui::End();
		}

		if (m_SettingMenuEnable[SSR])
		{
			ImGui::Begin(SettingMenuName[SSR], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::ScreenSpaceReflection.GetEnable());
			ImGui::Checkbox("DebugDraw", &KRenderGlobal::ScreenSpaceReflection.GetDebugDrawEnable());
			ImGui::SliderInt("RayReuse", &KRenderGlobal::ScreenSpaceReflection.GetRayReuseCount(), 1, 9);
			ImGui::SliderInt("Atrous", &KRenderGlobal::ScreenSpaceReflection.GetAtrousLevel(), 0, 5);

			ImGui::End();
		}

		if (m_SettingMenuEnable[DOF])
		{
			ImGui::Begin(SettingMenuName[DOF], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("DebugDraw", &KRenderGlobal::DepthOfField.GetDebugDrawEnable());
			ImGui::SliderFloat("CocLimit", &KRenderGlobal::DepthOfField.GetCocLimit(), 0.001f, 1.0f);
			ImGui::SliderFloat("FStop", &KRenderGlobal::DepthOfField.GetFStop(), 0.50f, 128.0f);
			// ImGui::SliderFloat("FocalLength", &KRenderGlobal::DepthOfField.GetFocalLength(), 1.0f, 5000.0f);
			ImGui::SliderFloat("FocusDistance", &KRenderGlobal::DepthOfField.GetFocusDistance(), 1.0f, 5000.0f);
			// ImGui::SliderFloat("Near", &KRenderGlobal::DepthOfField.GetNearRange(), 1.0f, 5000.0f);
			ImGui::SliderFloat("Far", &KRenderGlobal::DepthOfField.GetFarRange(), 1.0f, 5000.0f);

			ImGui::End();
		}

		if (m_SettingMenuEnable[RTAO])
		{
			ImGui::Begin(SettingMenuName[RTAO], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::RTAO.GetEnable());
			ImGui::Checkbox("DebugDraw", &KRenderGlobal::RTAO.GetDebugDrawEnable());
			ImGui::SliderFloat("Length of the ray", &KRenderGlobal::RTAO.GetAoParameters().rtao_radius, 0.0f, 20.0f);
			ImGui::SliderFloat("Strenth of darkness", &KRenderGlobal::RTAO.GetAoParameters().rtao_power, 0.0001f, 10.0f);
			ImGui::SliderInt("Number of samples at each iteration", &KRenderGlobal::RTAO.GetAoParameters().rtao_samples, 1, 256);
			ImGui::SliderInt("Attenuate based on distance", &KRenderGlobal::RTAO.GetAoParameters().rtao_distance_based, 0, 1);

			ImGui::End();
		}

		if (m_SettingMenuEnable[SVO_GI])
		{
			ImGui::Begin(SettingMenuName[SVO_GI], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::Voxilzer.GetEnable());
			ImGui::Checkbox("Octree", &KRenderGlobal::Voxilzer.GetVoxelUseOctree());
			ImGui::Checkbox("VoxelDraw", &KRenderGlobal::Voxilzer.GetVoxelDrawEnable());
			ImGui::Checkbox("VoxelDrawWireFrame", &KRenderGlobal::Voxilzer.GetVoxelDrawWireFrame());
			ImGui::Checkbox("LightDraw", &KRenderGlobal::Voxilzer.GetLightDebugDrawEnable());
			ImGui::Checkbox("OctreeRayTestDraw", &KRenderGlobal::Voxilzer.GetOctreeRayTestDrawEnable());

			ImGui::End();
		}
		
		if (m_SettingMenuEnable[CLIPMAP_GI])
		{
			ImGui::Begin(SettingMenuName[CLIPMAP_GI], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::ClipmapVoxilzer.GetEnable());
			ImGui::Checkbox("LightDraw", &KRenderGlobal::ClipmapVoxilzer.GetLightDebugDrawEnable());
			ImGui::Checkbox("VoxelDebugUpdate", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDebugUpdate());
			ImGui::Checkbox("VoxelDebugVoxelize", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDebugVoxelize());
			ImGui::Checkbox("VoxelDraw", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDrawEnable());
			ImGui::Checkbox("VoxelDrawWireFrame", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDrawWireFrame());
			ImGui::SliderFloat("VoxelBias", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDrawBias(), 0, 16);
			ImGui::End();
		}

		if (m_SettingMenuEnable[HIZ_OC])
		{
			ImGui::Begin(SettingMenuName[HIZ_OC], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::HiZOcclusion.GetEnable());
			ImGui::Checkbox("DebugDraw", &KRenderGlobal::HiZOcclusion.GetDebugDrawEnable());

			ImGui::End();
		}

		if (m_SettingMenuEnable[HARDWARE_OC])
		{
			ImGui::Begin(SettingMenuName[HARDWARE_OC], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::OcclusionBox.GetEnable());
			ImGui::SliderFloat("DepthBiasConstant", &KRenderGlobal::OcclusionBox.GetDepthBiasConstant(), -5.0f, 5.0f);
			ImGui::SliderFloat("DepthBiasSlope", &KRenderGlobal::OcclusionBox.GetDepthBiasSlope(), -5.0f, 5.0f);
			ImGui::SliderFloat("Instance Size", &KRenderGlobal::OcclusionBox.GetInstanceGroupSize(), 10.0f, 100000.0f);
		
			ImGui::End();
		}

		if (m_SettingMenuEnable[VOLUMETIRIC_FOG])
		{
			ImGui::Begin(SettingMenuName[VOLUMETIRIC_FOG], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::VolumetricFog.GetEnable());
			ImGui::SliderFloat("Start", &KRenderGlobal::VolumetricFog.GetStart(), 1.0f, 5000.0f);
			ImGui::SliderFloat("Depth", &KRenderGlobal::VolumetricFog.GetDepth(), 1.0f, 5000.0f);
			ImGui::SliderFloat("Anisotropy", &KRenderGlobal::VolumetricFog.GetAnisotropy(), 0.0f, 1.0f);
			ImGui::SliderFloat("Density", &KRenderGlobal::VolumetricFog.GetDensity(), 0.0f, 1.0f);

			ImGui::End();
		}

		if (m_SettingMenuEnable[VIRTUAL_GEOMETRY])
		{
			ImGui::Checkbox("UseMeshPipeline", &KRenderGlobal::VirtualGeometryManager.GetUseMeshPipeline());
			ImGui::Checkbox("UseDoubleOcclusion", &KRenderGlobal::VirtualGeometryManager.GetUseDoubleOcclusion());
			ImGui::Checkbox("UsePersistentCull", &KRenderGlobal::VirtualGeometryManager.GetUsePersistentCull());
		}
	}
	ImGui::PopStyleColor(1);
}