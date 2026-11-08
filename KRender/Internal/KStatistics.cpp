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
	m_DrawStatistics.stages.push_back({ "Uncategorized", {} });
}

KStatistics::~KStatistics()
{
}

KDrawStatsStageId KStatistics::RegisterDrawStatsStage(const char* name)
{
	assert(name && *name);
	if (!name || !*name)
	{
		return K_NO_DRAW_STATS_STAGE;
	}

	std::lock_guard<std::mutex> lock(m_Mutex);
	auto& stages = m_DrawStatistics.stages;
	auto it = std::find_if(stages.begin(), stages.end(), [name](const KRenderDrawStageStatistics& stage)
	{
		return stage.name == name;
	});
	if (it != stages.end())
	{
		return static_cast<KDrawStatsStageId>(it - stages.begin());
	}

	const auto id = static_cast<KDrawStatsStageId>(stages.size());
	stages.push_back({ name, {} });
	return id;
}

void KStatistics::BeginDrawFrame(uint32_t frameNumber)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	assert(!m_DrawFrameActive);
	m_PendingDrawStages.clear();
	m_PendingDrawFrame = frameNumber;
	m_DrawFrameActive = true;
}

void KStatistics::AccumulateSubmittedDraws(const std::vector<KRenderStageStatistics>& stages)
{
	if (stages.empty())
	{
		return;
	}

	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_DrawFrameActive)
	{
		assert(stages.size() <= m_DrawStatistics.stages.size());
		m_PendingDrawStages.resize(std::max(stages.size(), m_PendingDrawStages.size()));
		for (size_t i = 0; i < stages.size(); ++i)
		{
			m_PendingDrawStages[i] += stages[i];
		}
	}
}

void KStatistics::EndDrawFrame()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	assert(m_DrawFrameActive);
	m_DrawStatistics.frameNumber = m_PendingDrawFrame;
	m_DrawStatistics.total = {};
	for (size_t i = 0; i < m_DrawStatistics.stages.size(); ++i)
	{
		auto& counts = m_DrawStatistics.stages[i].counts;
		counts = i < m_PendingDrawStages.size() ? m_PendingDrawStages[i] : KRenderStageStatistics{};
		m_DrawStatistics.total += counts;
	}
	m_DrawStatistics.valid = true;
	m_DrawFrameActive = false;
}

void KStatistics::ResetDrawStatistics()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_DrawFrameActive = false;
	m_PendingDrawStages.clear();
	m_DrawStatistics.valid = false;
	m_DrawStatistics.frameNumber = 0;
	m_DrawStatistics.total = {};
	for (auto& stage : m_DrawStatistics.stages)
	{
		stage.counts = {};
	}
}

void KStatistics::Update()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
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

KRenderFrameStatistics KStatistics::GetFrameStatistics() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_FrameData.current;
}

bool KStatistics::GetAllStatistics(KRenderStatistics& statistics)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	statistics.frame = m_FrameData.current;
	statistics.draw = m_DrawStatistics;
	return true;
}

float KStatistics::GetRefreshTime() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_RefreshTime;
}

void KStatistics::SetRefreshTime(float time)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_RefreshTime = time;
}

float KStatistics::GetMaxMinRefreshTime() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_MaxMinRefreshTime;
}

void KStatistics::SetMaxMinRefreshTime(float time)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_MaxMinRefreshTime = time;
}