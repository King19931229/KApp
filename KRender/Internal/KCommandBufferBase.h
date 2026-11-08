#pragma once
#include "Interface/IKCommandBuffer.h"

class KCommandBufferBase : public IKCommandBuffer
{
	struct DrawRecording
	{
		std::vector<KDrawStatsStageId> stageStack;
		std::vector<KRenderStageStatistics> stages;
		size_t passStackDepth = 0;
		bool passHasStage = false;

		KDrawStatsStageId CurrentStage() const;
		KRenderStageStatistics& Counts(KDrawStatsStageId stage);
	};
	std::vector<DrawRecording> m_DrawRecordings;
	DrawRecording& GetDrawRecording();

protected:
	void InitDrawRecordings(size_t count) { m_DrawRecordings.resize(count); }
	void ReleaseDrawRecordings() { m_DrawRecordings.clear(); }
	void ResetDrawRecording();
	void BeginRenderPassDrawStats(KDrawStatsStageId stage);
	void EndRenderPassDrawStats();
	bool HasOpenDrawStatsScopes();
	void RecordDirectDraw(PrimitiveTopology topology, uint32_t elementCount, uint32_t instanceCount);
	void MergeSecondaryDraws(KCommandBufferBase& secondary);

public:
	void BeginDrawStatsStage(KDrawStatsStageId stage) override;
	void EndDrawStatsStage() override;
	void ReportSubmittedDraws();
};
