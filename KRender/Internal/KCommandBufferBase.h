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
	DrawRecording m_DrawRecording;
	DrawRecording& GetDrawRecording();

protected:
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
