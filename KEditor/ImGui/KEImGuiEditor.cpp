#include "KEImGuiEditor.h"
#include "KEditorGlobal.h"
#include "KBase/Interface/Task/IKTaskGraph.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Interface/IKLog.h"
#include "KRender/Publish/KRenderApplication.h"
#include "KRender/Interface/IKPostProcess.h"
#include "KRender/Internal/KRenderThread.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <windows.h>
#include <commdlg.h>

namespace KRenderGlobal { extern bool InEditor; }

namespace
{
	std::string PropertyKey(const std::vector<std::string>& path)
	{
		std::string key;
		for (const auto& part : path) { key += "/"; key += part; }
		return key;
	}

	KReflectionObjectBase* ResolveObject(KReflectionObjectBase* object, const std::vector<std::string>& path)
	{
		for (size_t i = 0; object && i + 1 < path.size(); ++i)
		{
			auto prop = KRTTR_GET_TYPE(object).get_property(path[i]);
			if (!prop.is_valid()) return nullptr;
			auto value = prop.get_value(object);
			auto type = prop.get_metadata(META_DATA_TYPE);
			if (!value.is_valid() || !type.is_valid() || type.get_value<MetaDataType>() != MDT_OBJECT) return nullptr;
			object = value.get_value<KReflectionObjectBase*>();
		}
		return object;
	}

	bool MatchesFilter(const std::string& value, const char* filter)
	{
		std::string a = value, b = filter;
		std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return a.find(b) != std::string::npos;
	}
}

bool KEImGuiEditor::Init(RenderWindowType windowType, size_t width, size_t height, const char* scene)
{
	KRenderGlobal::InEditor = true;
	KEngineGlobal::CreateEngine();
	m_Engine = KEngineGlobal::Engine;
	KEngineOptions options;
	options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_DEFAULT;
	options.window.width = width;
	options.window.height = height;
	options.window.left = 100;
	options.window.top = 60;
	IKRenderWindowPtr window = CreateRenderWindow(windowType);
	m_Window = window.get();
	if (!m_Engine || !window || !m_Engine->Init(std::move(window), options))
	{
		if (m_Engine) m_Engine->UnInit();
		m_Engine.reset();
		m_Window = nullptr;
		KEngineGlobal::DestroyEngine();
		KRenderGlobal::InEditor = false;
		return false;
	}
	m_Window->SetWindowTitle("KEditor");
	IKRenderCore* core = m_Engine->GetRenderCore();
	KEditorGlobal::EntityNamePool.Init();
	KEditorGlobal::EntitySelector.Init();
	if (!KEditorGlobal::EntityManipulator.Init(core->GetGizmo(), m_Window, core->GetCamera(), m_Engine->GetScene()))
	{
		UnInit();
		return false;
	}
	if (scene && *scene && !KEditorGlobal::EntityManipulator.Load(scene))
	{
		UnInit();
		return false;
	}
	IKFileSystemPtr resources = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);
	if (resources) resources->GetAllSubFileSystem(m_ResourceSystems);
	RefreshResources();
	RefreshSnapshot();
	m_AcceptActions = true;
	m_DrawCallback = [this]() { Draw(); };
	if (!core->RegisterUIRenderCallback(&m_DrawCallback))
	{
		UnInit();
		return false;
	}
	return true;
}

bool KEImGuiEditor::Run(uint32_t frames, const char* output)
{
	if (m_Window->GetType() == RENDER_WINDOW_HEADLESS)
	{
		return KRenderApplication::RunHeadless(*m_Engine->GetRenderCore(), frames, output, [this]() { RefreshSnapshot(); });
	}
	while (!m_Engine->GetRenderCore()->TickShouldEnd())
	{
		FLUSH_RENDER_COMMAND();
		GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::GAME_THREAD);
		RefreshSnapshot();
		m_Engine->Tick();
	}
	return true;
}

void KEImGuiEditor::UnInit()
{
	if (!m_Engine) return;
	m_Engine->Wait();
	m_Engine->GetRenderCore()->UnRegisterUIRenderCallback(&m_DrawCallback);
	m_AcceptActions = false;
	GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::GAME_THREAD);
	KEditorGlobal::EntitySelector.Clear();
	m_Engine->GetScene()->Clear();
	KEditorGlobal::CommandInvoker.Clear();
	KEditorGlobal::EntityManipulator.UnInit();
	KEditorGlobal::EntitySelector.UnInit();
	KEditorGlobal::EntityNamePool.UnInit();
	m_ResourceSystems.clear();
	m_Engine->UnInit();
	m_Engine.reset();
	m_Window = nullptr;
	KEngineGlobal::DestroyEngine();
	KRenderGlobal::InEditor = false;
}

void KEImGuiEditor::PostGame(std::function<void()> action)
{
	if (m_AcceptActions)
	{
		GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KLambdaTaskWork([this, action]()
		{
			if (m_AcceptActions) action();
		})), NamedThread::GAME_THREAD, {});
	}
}

void KEImGuiEditor::RefreshResources()
{
	m_Resources.clear();
	if (m_ResourceRoot < 0 || m_ResourceRoot >= (int)m_ResourceSystems.size()) return;
	IKFileSystemPtr system = m_ResourceSystems[m_ResourceRoot];
	std::vector<std::string> entries;
	if (system->ListDir(m_Folder, entries))
	{
		for (const std::string& name : entries)
		{
			if (name == "." || name == "..") continue;
			std::string path;
			KFileTool::PathJoin(m_Folder, name, path);
			m_Resources.push_back({ name, path, system->IsDir(path) });
		}
		std::sort(m_Resources.begin(), m_Resources.end(), [](const Resource& a, const Resource& b)
		{
			return a.directory != b.directory ? a.directory : a.name < b.name;
		});
	}
}

void KEImGuiEditor::CollectProperties(KReflectionObjectBase* object, std::vector<std::string>& path, std::vector<Property>& out)
{
	if (!object) return;
	for (auto prop : KRTTR_GET_TYPE(object).get_properties())
	{
		auto meta = prop.get_metadata(META_DATA_TYPE);
		auto value = prop.get_value(object);
		if (!meta.is_valid() || !value.is_valid()) continue;
		path.push_back(prop.get_name().to_string());
		MetaDataType type = meta.get_value<MetaDataType>();
		if (type == MDT_OBJECT)
		{
			CollectProperties(value.get_value<KReflectionObjectBase*>(), path, out);
		}
		else
		{
			Property property;
			property.path = path;
			property.type = type;
			property.readOnly = prop.is_readonly();
			switch (type)
			{
			case MDT_INT: property.integer = value.get_value<int>(); break;
			case MDT_FLOAT: property.value[0] = value.get_value<float>(); break;
			case MDT_STDSTRING: property.text = value.get_value<std::string>(); break;
			case MDT_FLOAT2: { auto v = value.get_value<glm::vec2>(); for (int i = 0; i < 2; ++i) property.value[i] = v[i]; break; }
			case MDT_FLOAT3: { auto v = value.get_value<glm::vec3>(); for (int i = 0; i < 3; ++i) property.value[i] = v[i]; break; }
			case MDT_FLOAT4: { auto v = value.get_value<glm::vec4>(); for (int i = 0; i < 4; ++i) property.value[i] = v[i]; break; }
			default: path.pop_back(); continue;
			}
			out.push_back(std::move(property));
		}
		path.pop_back();
	}
}

void KEImGuiEditor::RefreshSnapshot()
{
	auto snapshot = std::make_shared<Snapshot>();
	const auto& selected = KEditorGlobal::EntitySelector.GetSelection();
	for (const auto& item : KEditorGlobal::EntityManipulator.GetEntities())
	{
		snapshot->entities.push_back({ item.first, item.second->soul->GetName(), selected.find(item.first) != selected.end() });
	}
	std::sort(snapshot->entities.begin(), snapshot->entities.end(), [](const Entity& a, const Entity& b) { return a.name < b.name; });
	for (const auto& item : selected) snapshot->selection.push_back(item.first);
	if (!selected.empty())
	{
		KReflectionObjectBase* first = nullptr;
		selected.begin()->second->soul->QueryReflection(&first);
		std::vector<std::string> path;
		CollectProperties(first, path, snapshot->properties);
		for (auto it = snapshot->properties.begin(); it != snapshot->properties.end();)
		{
			bool common = true;
			for (const auto& entry : selected)
			{
				KReflectionObjectBase* root = nullptr;
				entry.second->soul->QueryReflection(&root);
				KReflectionObjectBase* object = ResolveObject(root, it->path);
				if (!object) { common = false; break; }
				auto prop = KRTTR_GET_TYPE(object).get_property(it->path.back());
				if (!prop.is_valid()) { common = false; break; }
				auto meta = prop.get_metadata(META_DATA_TYPE);
				if (!meta.is_valid() || meta.get_value<MetaDataType>() != it->type) { common = false; break; }
				it->readOnly |= prop.is_readonly();
			}
			if (!common) it = snapshot->properties.erase(it); else ++it;
		}
	}
	for (const auto& system : m_ResourceSystems)
	{
		std::string root;
		system->GetRoot(root);
		snapshot->roots.push_back(root);
	}
	snapshot->root = m_ResourceRoot;
	snapshot->folder = m_Folder;
	snapshot->resources = m_Resources;
	std::lock_guard<std::mutex> lock(m_SnapshotMutex);
	m_Snapshot = std::move(snapshot);
}

void KEImGuiEditor::ApplyProperty(const std::vector<size_t>& ids, const Property& property)
{
	for (size_t id : ids)
	{
		KEEntityPtr entity = KEditorGlobal::EntityManipulator.GetEntity(id);
		if (!entity) continue;
		KReflectionObjectBase* root = nullptr;
		entity->soul->QueryReflection(&root);
		KReflectionObjectBase* object = ResolveObject(root, property.path);
		if (!object) continue;
		auto prop = KRTTR_GET_TYPE(object).get_property(property.path.back());
		if (!prop.is_valid() || prop.is_readonly()) continue;
		switch (property.type)
		{
		case MDT_INT: prop.set_value(*object, property.integer); break;
		case MDT_FLOAT: prop.set_value(*object, property.value[0]); break;
		case MDT_STDSTRING: prop.set_value(*object, property.text); break;
		case MDT_FLOAT2: prop.set_value(*object, glm::vec2(property.value[0], property.value[1])); break;
		case MDT_FLOAT3: prop.set_value(*object, glm::vec3(property.value[0], property.value[1], property.value[2])); break;
		case MDT_FLOAT4: prop.set_value(*object, glm::vec4(property.value[0], property.value[1], property.value[2], property.value[3])); break;
		default: break;
		}
		auto notify = prop.get_metadata(META_DATA_NOTIFY);
		if (notify.is_valid() && notify.get_value<MetaDataNotify>() == MDN_EDITOR) KEditorGlobal::EntityManipulator.UpdateGizmoTransform();
	}
}

void KEImGuiEditor::OpenScene(bool save)
{
	if (m_Window->GetType() == RENDER_WINDOW_HEADLESS)
	{
		KG_LOGE(LM_RENDER, "Native file dialogs are unavailable for a headless window");
		return;
	}
	wchar_t path[32768] = {};
	OPENFILENAMEW dialog = {};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner = (HWND)m_Window->GetHWND();
	dialog.lpstrFilter = L"Scene files (*.scene)\0*.scene\0All files\0*.*\0\0";
	dialog.lpstrFile = path;
	dialog.nMaxFile = (DWORD)ARRAY_SIZE(path);
	dialog.lpstrDefExt = L"scene";
	dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
	if (!(save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))) return;
	int size = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
	if (size <= 0) return;
	std::string name(size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, path, -1, &name[0], size, nullptr, nullptr);
	name.pop_back();
	m_Engine->Wait();
	if (save) KEditorGlobal::EntityManipulator.Save(name.c_str());
	else KEditorGlobal::EntityManipulator.Load(name.c_str());
}

void KEImGuiEditor::DrawDockspace()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGuiID dockspace = ImGui::GetID("KEditorDockspace");
	if (!ImGui::DockBuilderGetNode(dockspace))
	{
		ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace, viewport->WorkSize);
		ImGuiID center = dockspace;
		ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.20f, nullptr, &center);
		ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
		ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.27f, nullptr, &center);
		ImGui::DockBuilderDockWindow("Scene", left);
		ImGui::DockBuilderDockWindow("Properties", right);
		ImGui::DockBuilderDockWindow("Resources", bottom);
		ImGui::DockBuilderDockWindow("Viewport", center);
		ImGui::DockBuilderFinish(dockspace);
	}
	ImGui::DockSpaceOverViewport(dockspace, viewport);
}

void KEImGuiEditor::Draw()
{
	std::shared_ptr<const Snapshot> snapshot;
	{
		std::lock_guard<std::mutex> lock(m_SnapshotMutex);
		snapshot = m_Snapshot;
	}
	if (!snapshot) return;
	DrawDockspace();
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Load scene...")) PostGame([this]() { OpenScene(false); });
			if (ImGui::MenuItem("Save scene...")) PostGame([this]() { OpenScene(true); });
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) PostGame([]() { KEditorGlobal::CommandInvoker.Undo(); });
			if (ImGui::MenuItem("Redo", "Ctrl+Y")) PostGame([]() { KEditorGlobal::CommandInvoker.Redo(); });
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Post-process graph", nullptr, &m_ShowGraph);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	DrawScene(*snapshot);
	DrawProperties(*snapshot);
	DrawResources(*snapshot);
	m_GraphFocused = false;
	if (m_ShowGraph) DrawGraph();
	DrawViewport();
	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantTextInput && !m_GraphFocused)
	{
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) PostGame([]() { KEditorGlobal::CommandInvoker.Undo(); });
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) PostGame([]() { KEditorGlobal::CommandInvoker.Redo(); });
		if ((m_SceneFocused || m_ViewportFocused) && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
		{
			const auto ids = snapshot->selection;
			PostGame([ids]()
			{
				std::vector<KEEntityPtr> entities;
				for (size_t id : ids) if (auto entity = KEditorGlobal::EntityManipulator.GetEntity(id)) entities.push_back(entity);
				KEditorGlobal::EntityManipulator.Erase(entities);
			});
		}
	}
}

void KEImGuiEditor::DrawScene(const Snapshot& snapshot)
{
	ImGui::Begin("Scene");
	m_SceneFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	ImGui::InputTextWithHint("##filter", "Search entities", m_SceneFilter, ARRAY_SIZE(m_SceneFilter));
	for (const Entity& entity : snapshot.entities)
	{
		if (!MatchesFilter(entity.name, m_SceneFilter)) continue;
		ImGui::PushID((void*)(uintptr_t)entity.id);
		if (ImGui::Selectable(entity.name.c_str(), entity.selected))
		{
			bool multi = ImGui::GetIO().KeyCtrl;
			PostGame([id = entity.id, multi]()
			{
				auto entity = KEditorGlobal::EntityManipulator.GetEntity(id);
				if (!entity) return;
				bool wasSelected = KEditorGlobal::EntitySelector.Contain(id);
				if (!multi) KEditorGlobal::EntitySelector.Clear();
				if (multi && wasSelected) KEditorGlobal::EntitySelector.Remove(id);
				else KEditorGlobal::EntitySelector.Add(entity);
			});
		}
		ImGui::PopID();
	}
	ImGui::End();
}

void KEImGuiEditor::DrawProperties(const Snapshot& snapshot)
{
	ImGui::Begin("Properties");
	if (snapshot.selection.empty()) ImGui::TextDisabled("Select an entity");
	else if (snapshot.selection.size() > 1) ImGui::Text("%zu entities selected", snapshot.selection.size());
	for (const Property& property : snapshot.properties)
	{
		std::string key = PropertyKey(property.path);
		PropertyEdit& edit = m_PropertyEdits[key];
		if (!edit.active)
		{
			edit.integer = property.integer;
			std::copy(property.value, property.value + 4, edit.value);
			strncpy_s(edit.text, property.text.c_str(), _TRUNCATE);
		}
		std::string label;
		for (const auto& part : property.path) { if (!label.empty()) label += "."; label += part; }
		ImGui::PushID(key.c_str());
		ImGui::BeginDisabled(property.readOnly);
		bool changed = false;
		switch (property.type)
		{
		case MDT_INT: changed = ImGui::InputInt(label.c_str(), &edit.integer); break;
		case MDT_FLOAT: changed = ImGui::DragFloat(label.c_str(), edit.value, 0.1f); break;
		case MDT_STDSTRING: changed = ImGui::InputText(label.c_str(), edit.text, ARRAY_SIZE(edit.text), ImGuiInputTextFlags_EnterReturnsTrue); break;
		case MDT_FLOAT2: changed = ImGui::DragFloat2(label.c_str(), edit.value, 0.1f); break;
		case MDT_FLOAT3: changed = ImGui::DragFloat3(label.c_str(), edit.value, 0.1f); break;
		case MDT_FLOAT4: changed = ImGui::DragFloat4(label.c_str(), edit.value, 0.1f); break;
		default: break;
		}
		if (property.type == MDT_STDSTRING && ImGui::IsItemDeactivatedAfterEdit()) changed = true;
		edit.active = ImGui::IsItemActive();
		ImGui::EndDisabled();
		if (changed)
		{
			Property updated = property;
			updated.integer = edit.integer;
			std::copy(edit.value, edit.value + 4, updated.value);
			updated.text = edit.text;
			const auto ids = snapshot.selection;
			PostGame([this, ids, updated]() { ApplyProperty(ids, updated); });
		}
		ImGui::PopID();
	}
	ImGui::End();
}

void KEImGuiEditor::DrawResources(const Snapshot& snapshot)
{
	ImGui::Begin("Resources");
	const char* rootName = snapshot.root >= 0 && snapshot.root < (int)snapshot.roots.size() ? snapshot.roots[snapshot.root].c_str() : "No resource filesystem";
	if (ImGui::BeginCombo("Root", rootName))
	{
		for (int i = 0; i < (int)snapshot.roots.size(); ++i)
		{
			if (ImGui::Selectable(snapshot.roots[i].c_str(), i == snapshot.root))
				PostGame([this, i]() { m_ResourceRoot = i; m_Folder.clear(); RefreshResources(); });
		}
		ImGui::EndCombo();
	}
	if (ImGui::Button("Up")) PostGame([this]() { std::string parent; KFileTool::ParentFolder(m_Folder, parent); m_Folder = parent == "." ? "" : parent; RefreshResources(); });
	ImGui::SameLine();
	if (ImGui::Button("Refresh")) PostGame([this]() { RefreshResources(); });
	ImGui::SameLine();
	ImGui::TextUnformatted(snapshot.folder.c_str());
	ImGui::InputTextWithHint("##resource-filter", "Search resources", m_ResourceFilter, ARRAY_SIZE(m_ResourceFilter));
	for (const Resource& resource : snapshot.resources)
	{
		if (!MatchesFilter(resource.name, m_ResourceFilter)) continue;
		std::string label = resource.directory ? "[+] " + resource.name : resource.name;
		if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) && resource.directory && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			PostGame([this, path = resource.path]() { m_Folder = path; RefreshResources(); });
		if (!resource.directory && ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("KEDITOR_MODEL", resource.path.c_str(), resource.path.size() + 1);
			ImGui::TextUnformatted(resource.name.c_str());
			ImGui::EndDragDropSource();
		}
	}
	ImGui::End();
}

void KEImGuiEditor::DrawViewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	bool visible = ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleVar();
	if (ImGui::Button("Move")) PostGame([]() { KEditorGlobal::EntityManipulator.SetGizmoType(GizmoType::GIZMO_TYPE_MOVE); });
	ImGui::SameLine();
	if (ImGui::Button("Rotate")) PostGame([]() { KEditorGlobal::EntityManipulator.SetGizmoType(GizmoType::GIZMO_TYPE_ROTATE); });
	ImGui::SameLine();
	if (ImGui::Button("Scale")) PostGame([]() { KEditorGlobal::EntityManipulator.SetGizmoType(GizmoType::GIZMO_TYPE_SCALE); });
	ImGui::SameLine();
	if (ImGui::Button("Local / World")) PostGame([]()
	{
		auto mode = KEditorGlobal::EntityManipulator.GetManipulateMode();
		KEditorGlobal::EntityManipulator.SetManipulateMode(mode == GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL ? GizmoManipulateMode::GIZMO_MANIPULATE_WORLD : GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL);
	});
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	if (ImGui::DragFloat("Speed", &m_CameraSpeed, 0.05f, 0.01f, 100.0f))
		PostGame([this, speed = m_CameraSpeed]() { m_Engine->GetRenderCore()->GetCameraController()->SetSpeed(speed); });
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 size = ImGui::GetContentRegionAvail();
	KEditorViewport viewport;
	m_ViewportFocused = false;
	if (visible && size.x > 1 && size.y > 1 && !ImGui::IsWindowCollapsed())
	{
		uint64_t texture = m_Engine->GetRenderCore()->GetEditorViewportTexture();
		ImGui::Image((ImTextureID)texture, size);
		viewport.x = pos.x; viewport.y = pos.y; viewport.width = size.x; viewport.height = size.y;
		viewport.hovered = ImGui::IsItemHovered();
		viewport.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput &&
			(!ImGui::IsAnyItemActive() || ImGui::IsItemActive());
		m_ViewportFocused = viewport.focused;
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("KEDITOR_MODEL"))
			{
				std::string path((const char*)payload->Data);
				PostGame([this, path]() { KEditorGlobal::ResourcePorter.ModelDrop(m_Engine->GetRenderCore()->GetCamera(), path); });
			}
			ImGui::EndDragDropTarget();
		}
	}
	m_Engine->GetRenderCore()->SetEditorViewport(viewport);
	ImGui::End();
}
