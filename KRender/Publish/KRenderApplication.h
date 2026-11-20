#pragma once
#include "KBase/Publish/KConfig.h"
#include <cstdint>
#include <functional>

struct IKRenderCore;

namespace KRenderApplication
{
	// Resolve command-line paths against the caller's directory before locating resources.
	EXPORT_DLL bool PreparePaths(std::string& scene, std::string& output, std::string& error);
	EXPORT_DLL bool BakeStaticMeshLOD(const std::string& path, std::string& error);
	EXPORT_DLL void SetStaticMeshOptions(int lodOverride, bool gpuScene);
	EXPORT_DLL bool RunHeadless(IKRenderCore& core, uint32_t frames, const char* output,
		const std::function<void()>& beforeTick = {});
}
