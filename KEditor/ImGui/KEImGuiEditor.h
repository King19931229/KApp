#pragma once

#include "KEngine/Interface/IKEngine.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKReflection.h"
#include "imgui.h"
#include <atomic>
#include <mutex>
#include <unordered_map>

class KEImGuiEditor
{
	struct Property
	{
		std::vector<std::string> path;
		MetaDataType type = MDT_UNKNOWN;
		bool readOnly = false;
		int integer = 0;
		float value[4] = {};
		std::string text;
	};
	struct Entity
	{
		size_t id;
		std::string name;
		bool selected;
	};
	struct Resource
	{
		std::string name, path;
		bool directory;
	};
	struct Snapshot
	{
		std::vector<Entity> entities;
		std::vector<size_t> selection;
		std::vector<Property> properties;
		std::vector<std::string> roots;
		std::vector<Resource> resources;
		std::string folder;
		int root = 0;
	};
	struct PropertyEdit
	{
		bool active = false;
		int integer = 0;
		float value[4] = {};
		char text[1024] = {};
	};
	struct NodePosition { ImVec2 value = ImVec2(0, 0); };
	struct NodeEdit { char vertex[512] = {}, fragment[512] = {}; bool initialized = false; };

	IKEnginePtr m_Engine;
	IKRenderWindow* m_Window = nullptr;
	KRenderCoreUIRenderCallback m_DrawCallback;
	std::atomic_bool m_AcceptActions { false };
	std::mutex m_SnapshotMutex;
	std::shared_ptr<const Snapshot> m_Snapshot;
	KFileSystemPtrList m_ResourceSystems;
	std::vector<Resource> m_Resources;
	std::string m_Folder;
	int m_ResourceRoot = 0;
	char m_SceneFilter[256] = {};
	char m_ResourceFilter[256] = {};
	std::unordered_map<std::string, PropertyEdit> m_PropertyEdits;
	std::unordered_map<std::string, NodePosition> m_NodePositions;
	std::unordered_map<std::string, NodeEdit> m_NodeEdits;
	std::string m_SelectedNode, m_ConnectionNode;
	int m_ConnectionSlot = -1;
	bool m_ShowGraph = false;
	bool m_GraphFocused = false;
	bool m_SceneFocused = false, m_ViewportFocused = false;
	float m_CameraSpeed = 1.0f;

	void PostGame(std::function<void()> action);
	void RefreshResources();
	void RefreshSnapshot();
	void CollectProperties(KReflectionObjectBase* object, std::vector<std::string>& path, std::vector<Property>& out);
	void ApplyProperty(const std::vector<size_t>& ids, const Property& property);
	void Draw();
	void DrawDockspace();
	void DrawScene(const Snapshot& snapshot);
	void DrawProperties(const Snapshot& snapshot);
	void DrawResources(const Snapshot& snapshot);
	void DrawViewport();
	void DrawGraph();
	void OpenScene(bool save);
public:
	bool Init();
	void Run();
	void UnInit();
};
