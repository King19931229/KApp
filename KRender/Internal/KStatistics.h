#pragma once
#include "Interface/IKStatistics.h"
#include "KBase/Publish/KTimer.h"

#include <unordered_map>

class KStatistics : public IKStatistics
{
protected:
	struct StageData
	{
		KRenderStageStatistics current;
		KRenderStageStatistics max;
		KRenderStageStatistics min;

		KTimer timer;
		KTimer max_min_timer;

		uint32_t num_frames;

		StageData()
		{
			num_frames = 0;
		}
	};
	typedef std::unordered_map<std::string, StageData> StageDataMap;
	StageDataMap m_StageData;

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
	uint32_t m_NumFrame;

	float m_MaxMinRefreshTime;
	float m_RefreshTime;
public:
	KStatistics();
	~KStatistics();

	virtual bool RegisterRenderStage(const char* stage);
	virtual bool UnRegisterRenderStage(const char* stage);

	virtual bool UpdateRenderStageStatistics(const char* stage, const KRenderStageStatistics& statistics);
	virtual void Update();

	virtual bool GetStageStatistics(const char* stage, KRenderStageStatistics& statistics);
	virtual bool GetMaxStageStatistics(const char* stage, KRenderStageStatistics& statistics);
	virtual bool GetMinStageStatistics(const char* stage, KRenderStageStatistics& statistics);

	virtual bool GetAllStatistics(KRenderStatistics& statistics);

	// time in ms
	virtual float GetRefreshTime() const;
	virtual void SetRefreshTime(float time);
	virtual float GetMaxMinRefreshTime() const;
	virtual void SetMaxMinRefreshTime(float time);
};