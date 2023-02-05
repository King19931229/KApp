#include "KRenderImGui.h"
#include "KRenderGlobal.h"
#include "KBase/Interface/IKJson.h"
#include "imgui.h"

KRenderImGui GRenderImGui;

const char* KRenderImGui::MenuName[] =
{
	"CSM",
	"SSR",
	"DOF",
	"RTAO",
	"SVOGI",
	"ClipmapGI",
	"HiZOcclusion",
	"HardwareOcclusion",
	"VolumetricFog"
};

KRenderImGui::KRenderImGui()
{
	memset(m_MenuEnable, 0, sizeof(m_MenuEnable));
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
			for (uint32_t i = 0; i < MENU_ITEM_COUNT; ++i)
			{
				assert(MenuName[i]);
				if (jsonDoc->HasMember(MenuName[i]))
				{
					m_MenuEnable[i] = jsonDoc->GetMember(MenuName[i])->GetBool();
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
	for (uint32_t i = 0; i < MENU_ITEM_COUNT; ++i)
	{
		root->AddMember(MenuName[i], jsonDoc->CreateBool(m_MenuEnable[i]));
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
				for (uint32_t i = 0; i < MENU_ITEM_COUNT; ++i)
				{
					ImGui::MenuItem(MenuName[i], nullptr, &m_MenuEnable[i]);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		if (m_MenuEnable[CSM])
		{
			ImGui::Begin(MenuName[CSM], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

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

		if (m_MenuEnable[SSR])
		{
			ImGui::Begin(MenuName[SSR], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("DebugDraw", &KRenderGlobal::ScreenSpaceReflection.GetDebugDrawEnable());
			ImGui::SliderInt("RayReuse", &KRenderGlobal::ScreenSpaceReflection.GetRayReuseCount(), 1, 9);
			ImGui::SliderInt("Atrous", &KRenderGlobal::ScreenSpaceReflection.GetAtrousLevel(), 0, 5);

			ImGui::End();
		}

		if (m_MenuEnable[DOF])
		{
			ImGui::Begin(MenuName[DOF], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("DebugDraw", &KRenderGlobal::DepthOfField.GetDebugDrawEnable());
			ImGui::SliderFloat("CocLimit", &KRenderGlobal::DepthOfField.GetCocLimit(), 0.001f, 1.0f);
			ImGui::SliderFloat("FStop", &KRenderGlobal::DepthOfField.GetFStop(), 0.50f, 128.0f);
			// ImGui::SliderFloat("FocalLength", &KRenderGlobal::DepthOfField.GetFocalLength(), 1.0f, 5000.0f);
			ImGui::SliderFloat("FocusDistance", &KRenderGlobal::DepthOfField.GetFocusDistance(), 1.0f, 5000.0f);
			// ImGui::SliderFloat("Near", &KRenderGlobal::DepthOfField.GetNearRange(), 1.0f, 5000.0f);
			ImGui::SliderFloat("Far", &KRenderGlobal::DepthOfField.GetFarRange(), 1.0f, 5000.0f);

			ImGui::End();
		}

		if (m_MenuEnable[RTAO])
		{
			ImGui::Begin(MenuName[RTAO], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::RTAO.GetEnable());
			ImGui::Checkbox("DebugDraw", &KRenderGlobal::RTAO.GetDebugDrawEnable());
			ImGui::SliderFloat("Length of the ray", &KRenderGlobal::RTAO.GetAoParameters().rtao_radius, 0.0f, 20.0f);
			ImGui::SliderFloat("Strenth of darkness", &KRenderGlobal::RTAO.GetAoParameters().rtao_power, 0.0001f, 10.0f);
			ImGui::SliderInt("Number of samples at each iteration", &KRenderGlobal::RTAO.GetAoParameters().rtao_samples, 1, 32);
			ImGui::SliderInt("Attenuate based on distance", &KRenderGlobal::RTAO.GetAoParameters().rtao_distance_based, 0, 1);

			ImGui::End();
		}

		if (m_MenuEnable[SVO_GI])
		{
			ImGui::Begin(MenuName[SVO_GI], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Octree", &KRenderGlobal::Voxilzer.GetVoxelUseOctree());
			ImGui::Checkbox("VoxelDraw", &KRenderGlobal::Voxilzer.GetVoxelDrawEnable());
			ImGui::Checkbox("VoxelDrawWireFrame", &KRenderGlobal::Voxilzer.GetVoxelDrawWireFrame());
			ImGui::Checkbox("LightDraw", &KRenderGlobal::Voxilzer.GetLightDebugDrawEnable());
			ImGui::Checkbox("OctreeRayTestDraw", &KRenderGlobal::Voxilzer.GetOctreeRayTestDrawEnable());

			ImGui::End();
		}
		
		if (m_MenuEnable[CLIPMAP_GI])
		{
			ImGui::Begin(MenuName[CLIPMAP_GI], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("VoxelDraw", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDrawEnable());
			ImGui::Checkbox("VoxelDebug", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDebugUpdate());
			ImGui::Checkbox("VoxelDrawWireFrame", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDrawWireFrame());
			ImGui::Checkbox("LightDraw", &KRenderGlobal::ClipmapVoxilzer.GetLightDebugDrawEnable());
			ImGui::SliderFloat("VoxelBias", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDrawBias(), 0, 16);

			ImGui::End();
		}

		if (m_MenuEnable[HIZ_OC])
		{
			ImGui::Begin(MenuName[HIZ_OC], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::HiZOcclusion.GetEnable());
			ImGui::Checkbox("DebugDraw", &KRenderGlobal::HiZOcclusion.GetDebugDrawEnable());

			ImGui::End();
		}

		if (m_MenuEnable[HARDWARE_OC])
		{
			ImGui::Begin(MenuName[HARDWARE_OC], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Checkbox("Enable", &KRenderGlobal::OcclusionBox.GetEnable());
			ImGui::SliderFloat("DepthBiasConstant", &KRenderGlobal::OcclusionBox.GetDepthBiasConstant(), -5.0f, 5.0f);
			ImGui::SliderFloat("DepthBiasSlope", &KRenderGlobal::OcclusionBox.GetDepthBiasSlope(), -5.0f, 5.0f);
			ImGui::SliderFloat("Instance Size", &KRenderGlobal::OcclusionBox.GetInstanceGroupSize(), 10.0f, 100000.0f);
		
			ImGui::End();
		}

		if (m_MenuEnable[VOLUMETIRIC_FOG])
		{
			ImGui::Begin(MenuName[VOLUMETIRIC_FOG], nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::SliderFloat("Start", &KRenderGlobal::VolumetricFog.GetStart(), 1.0f, 5000.0f);
			ImGui::SliderFloat("Depth", &KRenderGlobal::VolumetricFog.GetDepth(), 1.0f, 5000.0f);
			ImGui::SliderFloat("Anisotropy", &KRenderGlobal::VolumetricFog.GetAnisotropy(), 0.0f, 1.0f);
			ImGui::SliderFloat("Density", &KRenderGlobal::VolumetricFog.GetDensity(), 0.0f, 1.0f);

			ImGui::End();
		}
	}
	ImGui::PopStyleColor(1);
}