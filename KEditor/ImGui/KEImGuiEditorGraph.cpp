#include "KEImGuiEditor.h"
#include "KRender/Interface/IKPostProcess.h"
#include "KBase/Publish/KPixelFormat.h"
#include <algorithm>
#include <queue>
#include <unordered_set>

void KEImGuiEditor::DrawGraph()
{
	IKPostProcessManager* manager = GetProcessManager();
	ImGui::SetNextWindowSize(ImVec2(850, 600), ImGuiCond_FirstUseEver);
	ImGui::Begin("Post-process graph", &m_ShowGraph);
	m_GraphFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	KPostProcessNodeSet nodeSet;
	KPostProcessConnectionSet connections;
	manager->GetAllNodes(nodeSet);
	manager->GetAllConnections(connections);
	std::vector<IKPostProcessNodePtr> nodes(nodeSet.begin(), nodeSet.end());
	std::sort(nodes.begin(), nodes.end(), [](const IKPostProcessNodePtr& a, const IKPostProcessNodePtr& b) { return a->ID() < b->ID(); });
	if (ImGui::Button("Add pass"))
	{
		auto node = manager->CreatePass();
		node->CastPass()->SetShader("postprocess/screenquad.vert", "postprocess/screenquad.frag");
		m_NodePositions[node->ID()].value = ImVec2(220.0f, 30.0f + (float)nodes.size() * 35.0f);
		m_SelectedNode = node->ID();
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete node") && !m_SelectedNode.empty())
	{
		auto node = manager->GetNode(m_SelectedNode);
		if (node && node != manager->GetStartPointPass())
		{
			manager->DeleteNode(node);
			m_NodePositions.erase(m_SelectedNode);
			m_NodeEdits.erase(m_SelectedNode);
			m_SelectedNode.clear();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Auto layout"))
	{
		std::unordered_map<std::string, int> indegree, level, row;
		for (auto node : nodes) { indegree[node->ID()] = 0; level[node->ID()] = 0; }
		for (auto conn : connections) if (conn->IsComplete()) ++indegree[conn->GetInputPortNode()->ID()];
		std::queue<std::string> pending;
		for (const auto& item : indegree) if (!item.second) pending.push(item.first);
		while (!pending.empty())
		{
			std::string id = pending.front(); pending.pop();
			for (auto conn : connections)
			{
				if (conn->IsComplete() && conn->GetOutputPortNode()->ID() == id)
				{
					std::string next = conn->GetInputPortNode()->ID();
					level[next] = std::max(level[next], level[id] + 1);
					if (--indegree[next] == 0) pending.push(next);
				}
			}
		}
		for (auto node : nodes)
		{
			int column = level[node->ID()];
			m_NodePositions[node->ID()].value = ImVec2(30.0f + column * 250.0f, 30.0f + row[std::to_string(column)]++ * 165.0f);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Construct")) manager->Construct();
	ImGui::SameLine();
	ImGui::TextDisabled("Click an output then an input; right-click an input to disconnect.");

	ImVec2 area = ImGui::GetContentRegionAvail();
	float canvasHeight = std::max(160.0f, area.y * 0.65f);
	ImGui::BeginChild("GraphCanvas", ImVec2(0, canvasHeight), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 nodeSize(180, 125);
	for (size_t i = 0; i < nodes.size(); ++i)
	{
		const auto id = nodes[i]->ID();
		if (m_NodePositions.find(id) == m_NodePositions.end()) m_NodePositions[id].value = ImVec2(30.0f + (float)i * 210.0f, 30.0f);
	}
	for (auto conn : connections)
	{
		if (!conn->IsComplete()) continue;
		auto out = m_NodePositions.find(conn->GetOutputPortNode()->ID());
		auto in = m_NodePositions.find(conn->GetInputPortNode()->ID());
		if (out == m_NodePositions.end() || in == m_NodePositions.end()) continue;
		ImVec2 a(origin.x + out->second.value.x + nodeSize.x, origin.y + out->second.value.y + 38 + conn->GetOutputSlot() * 22.0f);
		ImVec2 b(origin.x + in->second.value.x, origin.y + in->second.value.y + 38 + conn->GetInputSlot() * 22.0f);
		draw->AddBezierCubic(a, ImVec2(a.x + 65, a.y), ImVec2(b.x - 65, b.y), b, IM_COL32(120, 195, 255, 255), 2.0f);
	}
	for (auto node : nodes)
	{
		std::string id = node->ID();
		ImGui::PushID(id.c_str());
		ImVec2& position = m_NodePositions[id].value;
		ImVec2 top(origin.x + position.x, origin.y + position.y);
		ImVec2 end(top.x + nodeSize.x, top.y + nodeSize.y);
		draw->AddRectFilled(top, end, IM_COL32(40, 44, 53, 255), 5.0f);
		draw->AddRect(top, end, m_SelectedNode == id ? IM_COL32(255, 180, 65, 255) : IM_COL32(95, 100, 110, 255), 5.0f);
		draw->AddRectFilled(top, ImVec2(end.x, top.y + 25), IM_COL32(65, 70, 83, 255), 5.0f, ImDrawFlags_RoundCornersTop);
		draw->AddText(ImVec2(top.x + 10, top.y + 5), IM_COL32_WHITE, node == manager->GetStartPointPass() ? "Scene output" : "Post-process pass");
		ImGui::SetCursorScreenPos(top);
		ImGui::InvisibleButton("header", ImVec2(nodeSize.x, 25));
		if (ImGui::IsItemClicked()) m_SelectedNode = id;
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			position.x = std::max(0.0f, position.x + ImGui::GetIO().MouseDelta.x);
			position.y = std::max(0.0f, position.y + ImGui::GetIO().MouseDelta.y);
		}
		for (int slot = 0; slot < PostProcessPort::MAX_INPUT_SLOT_COUNT; ++slot)
		{
			float y = top.y + 38 + slot * 22.0f;
			if (node != manager->GetStartPointPass())
			{
				draw->AddCircleFilled(ImVec2(top.x, y), 5, IM_COL32(120, 195, 255, 255));
				draw->AddText(ImVec2(top.x + 10, y - 7), IM_COL32_WHITE, std::to_string(slot).c_str());
				ImGui::PushID(slot);
				ImGui::SetCursorScreenPos(ImVec2(top.x - 8, y - 8));
				ImGui::InvisibleButton("in", ImVec2(16, 16));
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
					for (auto conn : connections) if (conn->IsComplete() && conn->GetInputPortNode() == node.get() && conn->GetInputSlot() == slot) manager->DeleteConnection(conn);
				if (ImGui::IsItemClicked() && !m_ConnectionNode.empty())
				{
					auto source = manager->GetNode(m_ConnectionNode);
					bool cycle = !source || source == node;
					std::unordered_set<std::string> visited;
					std::queue<std::string> pending;
					pending.push(id);
					while (!pending.empty() && !cycle)
					{
						std::string current = pending.front(); pending.pop();
						if (!visited.insert(current).second) continue;
						if (current == m_ConnectionNode) { cycle = true; break; }
						for (auto conn : connections)
							if (conn->IsComplete() && conn->GetOutputPortNode()->ID() == current) pending.push(conn->GetInputPortNode()->ID());
					}
					if (!cycle)
					{
						for (auto conn : connections) if (conn->IsComplete() && conn->GetInputPortNode() == node.get() && conn->GetInputSlot() == slot) manager->DeleteConnection(conn);
						manager->CreateConnection(source, (int16_t)m_ConnectionSlot, node, (int16_t)slot);
					}
					m_ConnectionNode.clear();
				}
				ImGui::PopID();
			}
			draw->AddCircleFilled(ImVec2(end.x, y), 5, IM_COL32(255, 180, 90, 255));
			draw->AddText(ImVec2(end.x - 20, y - 7), IM_COL32_WHITE, std::to_string(slot).c_str());
			ImGui::PushID(slot);
			ImGui::SetCursorScreenPos(ImVec2(end.x - 8, y - 8));
			if (ImGui::InvisibleButton("out", ImVec2(16, 16))) { m_ConnectionNode = id; m_ConnectionSlot = slot; }
			ImGui::PopID();
		}
		ImGui::PopID();
	}
	if (!m_ConnectionNode.empty())
	{
		auto it = m_NodePositions.find(m_ConnectionNode);
		if (it != m_NodePositions.end())
		{
			ImVec2 a(origin.x + it->second.value.x + nodeSize.x, origin.y + it->second.value.y + 38 + m_ConnectionSlot * 22.0f);
			ImVec2 b = ImGui::GetMousePos();
			draw->AddBezierCubic(a, ImVec2(a.x + 65, a.y), ImVec2(b.x - 65, b.y), b, IM_COL32(255, 180, 90, 255), 2.0f);
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_ConnectionNode.clear();
	}
	ImGui::SetCursorScreenPos(ImVec2(origin.x + 1600, origin.y + 1000));
	ImGui::Dummy(ImVec2(1, 1));
	ImGui::EndChild();

	auto selected = manager->GetNode(m_SelectedNode);
	if (selected && selected->CastPass())
	{
		auto pass = selected->CastPass();
		bool start = selected == manager->GetStartPointPass();
		ImGui::TextUnformatted(start ? "Scene output" : "Selected pass");
		float scale = pass->GetScale();
		if (ImGui::DragFloat("Scale", &scale, 0.01f, 0.05f, 4.0f)) pass->SetScale(scale);
		int msaa = pass->GetMSAA();
		const int sampleCounts[] = { 1, 2, 4, 8 };
		if (ImGui::BeginCombo("MSAA", std::to_string(msaa).c_str()))
		{
			for (int count : sampleCounts) if (ImGui::Selectable(std::to_string(count).c_str(), count == msaa)) pass->SetMSAA((unsigned short)count);
			ImGui::EndCombo();
		}
		int format = (int)pass->GetFormat();
		if (ImGui::BeginCombo("Format", KPixelFormatHelper::ToString((PixelFormat)format)))
		{
			for (int i = 0; i < PF_COUNT; ++i) if (ImGui::Selectable(KPixelFormatHelper::ToString((PixelFormat)i), i == format)) pass->SetFormat((PixelFormat)i);
			ImGui::EndCombo();
		}
		ImGui::BeginDisabled(start);
		NodeEdit& edit = m_NodeEdits[m_SelectedNode];
		if (!edit.initialized)
		{
			auto shaders = pass->GetShader();
			strncpy_s(edit.vertex, std::get<0>(shaders).c_str(), _TRUNCATE);
			strncpy_s(edit.fragment, std::get<1>(shaders).c_str(), _TRUNCATE);
			edit.initialized = true;
		}
		bool changed = ImGui::InputText("Vertex shader", edit.vertex, sizeof(edit.vertex), ImGuiInputTextFlags_EnterReturnsTrue);
		changed |= ImGui::IsItemDeactivatedAfterEdit();
		changed |= ImGui::InputText("Fragment shader", edit.fragment, sizeof(edit.fragment), ImGuiInputTextFlags_EnterReturnsTrue);
		changed |= ImGui::IsItemDeactivatedAfterEdit();
		if (changed) pass->SetShader(edit.vertex, edit.fragment);
		ImGui::EndDisabled();
	}
	ImGui::End();
}
