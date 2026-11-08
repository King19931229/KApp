#include "KCommandBufferBase.h"
#include "KRenderGlobal.h"

KCommandBufferBase::DrawRecording& KCommandBufferBase::GetDrawRecording()
{
	assert(KRenderGlobal::CurrentInFlightFrameIndex < m_DrawRecordings.size());
	return m_DrawRecordings[KRenderGlobal::CurrentInFlightFrameIndex];
}

KDrawStatsStageId KCommandBufferBase::DrawRecording::CurrentStage() const
{
	return stageStack.empty() ? K_NO_DRAW_STATS_STAGE : stageStack.back();
}

KRenderStageStatistics& KCommandBufferBase::DrawRecording::Counts(KDrawStatsStageId stage)
{
	if (stage >= stages.size())
	{
		stages.resize(size_t(stage) + 1);
	}
	return stages[stage];
}

void KCommandBufferBase::ResetDrawRecording()
{
	auto& recording = GetDrawRecording();
	recording.stageStack.clear();
	recording.stages.clear();
	recording.passStackDepth = 0;
	recording.passHasStage = false;
}

void KCommandBufferBase::BeginRenderPassDrawStats(KDrawStatsStageId stage)
{
	auto& recording = GetDrawRecording();
	recording.passHasStage = stage != K_NO_DRAW_STATS_STAGE;
	if (recording.passHasStage)
	{
		recording.passStackDepth = recording.stageStack.size();
		recording.stageStack.push_back(stage);
	}
}

void KCommandBufferBase::EndRenderPassDrawStats()
{
	auto& recording = GetDrawRecording();
	if (!recording.passHasStage)
	{
		return;
	}

	assert(recording.stageStack.size() == recording.passStackDepth + 1);
	if (recording.stageStack.size() > recording.passStackDepth)
	{
		recording.stageStack.resize(recording.passStackDepth);
	}
	recording.passHasStage = false;
}

void KCommandBufferBase::BeginDrawStatsStage(KDrawStatsStageId stage)
{
	auto& recording = GetDrawRecording();
	recording.stageStack.push_back(stage == K_NO_DRAW_STATS_STAGE ? recording.CurrentStage() : stage);
}

void KCommandBufferBase::EndDrawStatsStage()
{
	auto& stageStack = GetDrawRecording().stageStack;
	assert(!stageStack.empty());
	if (!stageStack.empty())
	{
		stageStack.pop_back();
	}
}

void KCommandBufferBase::RecordDirectDraw(PrimitiveTopology topology, uint32_t elementCount, uint32_t instanceCount)
{
	// Input assembly uses complete primitives and has primitive restart disabled.
	uint64_t perInstance = 0;
	bool triangle = false;
	switch (topology)
	{
	case PT_TRIANGLE_LIST:
		perInstance = elementCount / 3;
		triangle = true;
		break;
	case PT_TRIANGLE_STRIP:
		perInstance = elementCount > 2 ? elementCount - 2 : 0;
		triangle = true;
		break;
	case PT_POINT_LIST:
		perInstance = elementCount;
		break;
	case PT_LINE_LIST:
		perInstance = elementCount / 2;
		break;
	case PT_LINE_STRIP:
		perInstance = elementCount > 1 ? elementCount - 1 : 0;
		break;
	default:
		assert(false && "Unhandled draw topology");
		break;
	}

	auto& recording = GetDrawRecording();
	auto& counts = recording.Counts(recording.CurrentStage());
	const uint64_t primitives = perInstance * instanceCount;
	++counts.drawCalls;
	counts.primitives += primitives;
	if (triangle)
	{
		counts.triangles += primitives;
	}
}

void KCommandBufferBase::MergeSecondaryDraws(KCommandBufferBase& secondary)
{
	auto& recording = GetDrawRecording();
	const auto& stages = secondary.GetDrawRecording().stages;
	// Unmarked secondary draws inherit the stage at the execute site.
	for (KDrawStatsStageId id = 0; id < stages.size(); ++id)
	{
		if (stages[id].drawCalls != 0)
		{
			recording.Counts(id == K_NO_DRAW_STATS_STAGE ? recording.CurrentStage() : id) += stages[id];
		}
	}
}

bool KCommandBufferBase::HasOpenDrawStatsScopes()
{
	return !GetDrawRecording().stageStack.empty();
}

void KCommandBufferBase::ReportSubmittedDraws()
{
	KRenderGlobal::Statistics.AccumulateSubmittedDraws(GetDrawRecording().stages);
}
