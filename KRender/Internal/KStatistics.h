#pragma once
#include "Interface/IKStatistics.h"
#include "KBase/Publish/KTimer.h"

#include <mutex>

class KStatistics : public IKStatistics
{
protected:
	mutable std::mutex m_Mutex;
	KRenderDrawStatistics m_DrawStatistics;
	std::vector<KRenderStageStatistics> m_PendingDrawStages;
	uint32_t m_PendingDrawFrame = 0;
	bool m_DrawFrameActive = false;

	struct FrameData
	{
		KRenderFrameStatistics current;
		KRenderFrameStatistics max;
		KRenderFrameStatistics min;

		KTimer timer;
		KTimer max_min_timer;

		uint32_t num_frames;

		FrameData()
		{
			num_frames = 0;
		}
	};

	FrameData m_FrameData;

	float m_MaxMinRefreshTime;
	float m_RefreshTime;
public:
	KStatistics();
	~KStatistics();

	virtual KDrawStatsStageId RegisterDrawStatsStage(const char* name);
	void BeginDrawFrame(uint32_t frameNumber);
	void AccumulateSubmittedDraws(const std::vector<KRenderStageStatistics>& stages);
	void EndDrawFrame();
	void ResetDrawStatistics();

	virtual void Update();
	KRenderFrameStatistics GetFrameStatistics() const;
	virtual bool GetAllStatistics(KRenderStatistics& statistics);

	// time in ms
	virtual float GetRefreshTime() const;
	virtual void SetRefreshTime(float time);
	virtual float GetMaxMinRefreshTime() const;
	virtual void SetMaxMinRefreshTime(float time);
};