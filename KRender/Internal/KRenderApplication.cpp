#include "Publish/KRenderApplication.h"
#include "Interface/IKRenderCore.h"
#include "Interface/IKRenderWindow.h"
#include "Interface/IKSwapChain.h"
#include "Interface/IKFrameBuffer.h"
#include "Interface/IKRenderTarget.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KRenderThread.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Publish/KFileTool.h"
#include <atomic>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace KRenderApplication
{
	bool BakeStaticMeshLOD(const std::string& path, std::string& error)
	{
		return KMesh::BakeLOD(path, error);
	}

	void SetStaticMeshOptions(int lodOverride, bool gpuScene)
	{
		KRenderGlobal::StaticMeshLODOverride = lodOverride;
		KRenderGlobal::EnableGPUScene = gpuScene;
	}

	bool PreparePaths(std::string& scene, std::string& output, std::string& error)
	{
		error.clear();
		for (std::string* path : { &scene, &output })
		{
			if (path->empty()) continue;
#ifdef _WIN32
			DWORD length = GetFullPathNameA(path->c_str(), 0, nullptr, nullptr);
			std::string absolute(length, '\0');
			DWORD written = length ? GetFullPathNameA(path->c_str(), length, &absolute[0], nullptr) : 0;
			if (!written || written >= length)
			{
				error = "Cannot resolve path: " + *path;
				return false;
			}
			absolute.resize(written);
			*path = std::move(absolute);
#else
			if ((*path)[0] != '/')
			{
				char directory[4096];
				if (!getcwd(directory, sizeof(directory)))
				{
					error = "Cannot resolve the working directory";
					return false;
				}
				*path = std::string(directory) + "/" + *path;
			}
#endif
		}
		std::string directory;
#ifdef _WIN32
		if (!KFileTool::ExecFolder(directory) || !SetCurrentDirectoryA(directory.c_str()))
#else
		char executable[4096];
		ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
		if (length > 0) { executable[length] = '\0'; KFileTool::ParentFolder(executable, directory); }
		if (directory.empty() || chdir(directory.c_str()) != 0)
#endif
		{
			error = "Cannot select the executable's resource directory";
			return false;
		}
		if (!KFileTool::IsFile("../../../KConfig.ini"))
		{
			error = "Resource configuration not found: " + directory + "/../../../KConfig.ini";
			return false;
		}
		return true;
	}

	bool RunHeadless(IKRenderCore& core, uint32_t frames, const char* output, const std::function<void()>& beforeTick)
	{
		IKRenderWindow* window = core.GetMainWindow();
		if (!core.IsInit() || !frames || !window || window->GetType() != RENDER_WINDOW_HEADLESS) return false;
		IKSwapChain* swapChain = window->GetSwapChain();
		if (!swapChain) return false;
		std::atomic<uint32_t> completed { 0 }, lastImage { 0 };
		KDevicePresentCallback presented = [&](uint32_t image, uint32_t)
		{
			lastImage = image;
			++completed;
		};
		bool registered = false;
		ENQUEUE_RENDER_COMMAND(RegisterHeadlessPresent)([&]() { registered = swapChain->RegisterPostPresentCallback(&presented); });
		FLUSH_RENDER_COMMAND();
		if (!registered) return false;
		bool success = true;
		while (completed < frames)
		{
			FLUSH_RENDER_COMMAND();
			GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::GAME_THREAD);
			if (beforeTick) beforeTick();
			if (!core.Tick()) { success = false; break; }
			// Publish this frame's Present callback without waiting for GPU completion.
			ENQUEUE_RENDER_COMMAND(FlushHeadlessFrame)([]()
			{
				KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThread);
			});
			FLUSH_RENDER_COMMAND();
			if (core.TickShouldEnd()) { success = false; break; }
		}
		GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::GAME_THREAD);
		core.Wait();
		ENQUEUE_RENDER_COMMAND(UnregisterHeadlessPresent)([&]() { swapChain->UnRegisterPostPresentCallback(&presented); });
		FLUSH_RENDER_COMMAND();
		if (!success)
		{
			KG_LOGE(LM_RENDER, "Headless rendering ended after %u of %u frames", completed.load(), frames);
			return false;
		}
		if (output && *output)
		{
			KCodecResult image;
			bool captured = false;
			ENQUEUE_RENDER_COMMAND(ReadHeadlessImage)([&]()
			{
				IKFrameBufferPtr source = swapChain->GetColorFrameBuffer(lastImage.load());
				IKRenderTargetPtr readback;
				core.GetRenderDevice()->CreateRenderTarget(readback);
				image.eFormat = PF_R8G8B8A8_UNORM;
				image.uWidth = source->GetWidth();
				image.uHeight = source->GetHeight();
				if (readback->InitFromReadback((uint32_t)image.uWidth, (uint32_t)image.uHeight, 1, 1, image.eFormat) &&
					source->CopyToReadback(readback->GetFrameBuffer().get()))
				{
					image.pData = KImageDataPtr(KNEW KImageData(image.uWidth * image.uHeight * 4));
					KSubImageInfo subImage;
					subImage.uWidth = image.uWidth;
					subImage.uHeight = image.uHeight;
					subImage.uSize = image.pData->GetSize();
					image.pData->GetSubImageInfo().push_back(subImage);
					captured = readback->GetFrameBuffer()->Readback(image.pData->GetData(), image.pData->GetSize());
				}
				readback->UnInit();
			});
			FLUSH_RENDER_COMMAND();
			IKCodecPtr codec = KCodec::GetCodec(output);
			if (!captured || !codec || !codec->Save(image, output))
			{
				KG_LOGE(LM_RENDER, "Cannot save headless output: %s", output);
				return false;
			}
		}
		KG_LOG(LM_RENDER, "Headless completed %u frames", completed.load());
		return true;
	}
}
