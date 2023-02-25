#include "KStatistics.h"
#include "KRenderGlobal.h"
#include <algorithm>

IKStatistics* GetStatistics()
{
	return &KRenderGlobal::Statistics;
}

KStatistics::KStatistics()
	: m_MaxMinRefreshTime(5000.0f),
	m_RefreshTime(500.0f)
{
}

KStatistics::~KStatistics()
{
}

bool KStatistics::RegisterRenderStage(const char* stage)
{
	if (stage)
	{
		auto it = m_StageData.find(stage);
		if (it == m_StageData.end())
		{
			m_StageData[stage] = StageData();
		}
		return true;
	}
	return false;
}

bool KStatistics::UnRegisterRenderStage(const char* stage)
{
	if (stage)
	{
		auto it = m_StageData.find(stage);
		if (it != m_StageData.end())
		{
			m_StageData.erase(it);
			return true;
		}
	}
	return false;
}

bool KStatistics::ClearAllRenderStages()
{
	m_StageData.clear();
	return true;
}

bool KStatistics::UpdateRenderStageStatistics(const char* stage, const KRenderStageStatistics& statistics)
{
	if (stage)
	{
		auto it = m_StageData.find(stage);
		if (it != m_StageData.end())
		{
			StageData& stageData = it->second;

			stageData.current = statistics;

			if (stageData.max_min_timer.GetMilliseconds() > m_MaxMinRefreshTime)
			{
				stageData.max = statistics;
				stageData.min = statistics;
				stageData.max_min_timer.Reset();
			}

			++stageData.num_frames;

			float time = stageData.timer.GetMilliseconds();
			if (time > m_RefreshTime)
			{
#define UPDATE_MAX(data) stageData.max.data = std::max(stageData.max.data, stageData.current.data);
#define UPDATE_MIN(data) stageData.min.data = std::min(stageData.min.data, stageData.current.data);

				UPDATE_MAX(faces);
				UPDATE_MAX(primtives);
				UPDATE_MAX(drawcalls);

				UPDATE_MIN(faces);
				UPDATE_MIN(primtives);
				UPDATE_MIN(drawcalls);

#undef UPDATE_MIN
#undef UPDATE_MAX
				stageData.timer.Reset();
				stageData.num_frames = 0;
			}
			return true;
		}
	}
	return false;
}

void KStatistics::Update()
{
	if (m_FrameData.max_min_timer.GetMilliseconds() > m_MaxMinRefreshTime)
	{
		m_FrameData.max = m_FrameData.current;
		m_FrameData.min = m_FrameData.current;
		m_FrameData.max_min_timer.Reset();
	}

	++m_FrameData.num_frames;

	float time = m_FrameData.timer.GetMilliseconds();
	if (time > m_RefreshTime)
	{
		float frameTime = time / (float)m_FrameData.num_frames;
		float fps = 1000.0f / frameTime;

		m_FrameData.current.fps = fps;
		m_FrameData.current.frametime = frameTime;

#define UPDATE_MAX(data) m_FrameData.max.data = std::max(m_FrameData.max.data, m_FrameData.current.data);
#define UPDATE_MIN(data) m_FrameData.min.data = std::min(m_FrameData.min.data, m_FrameData.current.data);

		UPDATE_MAX(frametime);
		UPDATE_MAX(fps);

		UPDATE_MIN(frametime);
		UPDATE_MIN(fps);

#undef UPDATE_MIN
#undef UPDATE_MAX

		m_FrameData.timer.Reset();
		m_FrameData.num_frames = 0;
	}
}

bool KStatistics::GetStageStatistics(const char* stage, KRenderStageStatistics& statistics)
{
	if (stage)
	{
		auto it = m_StageData.find(stage);
		if (it != m_StageData.end())
		{
			statistics = it->second.current;
			return true;
		}
	}
	return false;
}

bool KStatistics::GetMaxStageStatistics(const char* stage, KRenderStageStatistics& statistics)
{
	if (stage)
	{
		auto it = m_StageData.find(stage);
		if (it != m_StageData.end())
		{
			statistics = it->second.max;
			return true;
		}
	}
	return false;
}

bool KStatistics::GetMinStageStatistics(const char* stage, KRenderStageStatistics& statistics)
{
	if (stage)
	{
		auto it = m_StageData.find(stage);
		if (it != m_StageData.end())
		{
			statistics = it->second.min;
			return true;
		}
	}
	return false;
}

bool KStatistics::GetAllStatistics(KRenderStatistics& statistics)
{
	statistics.stage.drawcalls = 0;
	statistics.stage.faces = 0;
	statistics.stage.primtives = 0;

	for (const auto& pair : m_StageData)
	{
		const KRenderStageStatistics& stage = pair.second.current;

		statistics.stage.drawcalls += stage.drawcalls;
		statistics.stage.faces += stage.faces;
		statistics.stage.primtives += stage.primtives;
	}

	statistics.frame = m_FrameData.current;

	return true;
}

float KStatistics::GetRefreshTime() const
{
	return m_RefreshTime;
}

void KStatistics::SetRefreshTime(float time)
{
	m_RefreshTime = time;
}

float KStatistics::GetMaxMinRefreshTime() const
{
	return m_MaxMinRefreshTime;
}

void KStatistics::SetMaxMinRefreshTime(float time)
{
	m_MaxMinRefreshTime = time;
}