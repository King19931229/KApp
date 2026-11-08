#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include <string>
#include <vector>

using KDrawStatsStageId = uint32_t;
// Zero means inherit the current stage. "Uncategorized" is reserved for draws with no stage.
constexpr KDrawStatsStageId K_NO_DRAW_STATS_STAGE = 0;

struct KRenderFrameStatistics
{
	float frametime;
	float fps;

	KRenderFrameStatistics()
	{
		frametime = 0;
		fps = 0;
	}
};

struct KRenderStageStatistics
{
	uint64_t drawCalls = 0;
	uint64_t primitives = 0;
	uint64_t triangles = 0;

	KRenderStageStatistics& operator+=(const KRenderStageStatistics& other)
	{
		drawCalls += other.drawCalls;
		primitives += other.primitives;
		triangles += other.triangles;
		return *this;
	}
};

struct KRenderDrawStageStatistics
{
	std::string name;
	KRenderStageStatistics counts;
};

struct KRenderDrawStatistics
{
	uint32_t frameNumber = 0;
	bool valid = false;
	KRenderStageStatistics total;
	// Indexed by KDrawStatsStageId, including the Uncategorized row at index zero.
	std::vector<KRenderDrawStageStatistics> stages;
};

struct KRenderStatistics
{
	KRenderFrameStatistics frame;
	KRenderDrawStatistics draw;
};

struct IKStatistics
{
	virtual ~IKStatistics() {}

	virtual KDrawStatsStageId RegisterDrawStatsStage(const char* name) = 0;
	virtual void Update() = 0;

	virtual bool GetAllStatistics(KRenderStatistics& statistics) = 0;

	// time in ms
	virtual float GetRefreshTime() const = 0;
	virtual void SetRefreshTime(float time) = 0;
	virtual float GetMaxMinRefreshTime() const = 0;
	virtual void SetMaxMinRefreshTime(float time) = 0;
};

EXPORT_DLL IKStatistics* GetStatistics();