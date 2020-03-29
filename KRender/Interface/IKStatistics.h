#pragma once
#include "Interface/IKRenderConfig.h"

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
	uint32_t faces;
	uint32_t primtives;
	uint32_t drawcalls;

	KRenderStageStatistics()
	{
		faces = 0;
		primtives = 0;
		drawcalls = 0;
	}
};

struct KRenderStatistics
{
	KRenderFrameStatistics frame;
	KRenderStageStatistics stage;
};

struct IKStatistics
{
	virtual ~IKStatistics() {}

	virtual bool RegisterRenderStage(const char* stage) = 0;
	virtual bool UnRegisterRenderStage(const char* stage) = 0;

	// call once each frame
	virtual bool UpdateRenderStageStatistics(const char* stage, const KRenderStageStatistics& statistics) = 0;
	virtual void Update() = 0;

	virtual bool GetStageStatistics(const char* stage, KRenderStageStatistics& statistics) = 0;
	virtual bool GetMaxStageStatistics(const char* stage, KRenderStageStatistics& statistics) = 0;
	virtual bool GetMinStageStatistics(const char* stage, KRenderStageStatistics& statistics) = 0;

	virtual bool GetAllStatistics(KRenderStatistics& statistics) = 0;

	// time in ms
	virtual float GetRefreshTime() const = 0;
	virtual void SetRefreshTime(float time) = 0;
	virtual float GetMaxMinRefreshTime() const = 0;
	virtual void SetMaxMinRefreshTime(float time) = 0;
};

EXPORT_DLL IKStatistics* GetStatistics();