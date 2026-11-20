#include "KEImGuiEditor.h"
#include "KBase/Publish/KCommandLine.h"
#include "KBase/Publish/KStringUtil.h"
#include "KRender/Publish/KRenderApplication.h"

int main(int argc, char** argv)
{
	KCommandLine commandLine;
	commandLine.AddBool("headless", false, false, "Render without a system window");
	commandLine.AddInt("width", 1600, "Output width");
	commandLine.AddInt("height", 900, "Output height");
	commandLine.AddInt("frames", 120, "Number of headless frames");
	commandLine.AddString("scene", "", "Scene file to load");
	commandLine.AddString("output", "", "Save the final headless frame as PNG");
	commandLine.AddString("bake-lod", "", "Bake LOD1 and LOD2 for a local glTF, GLB or OBJ");
	commandLine.AddString("lod", "auto", "Static mesh LOD mode (auto, 0, 1 or 2)");
	commandLine.AddBool("gpu-scene", false, false, "Enable GPUScene");
	commandLine.AddBool("help", false, false, "Show command-line help");
	if (!commandLine.Parse(argc, argv))
	{
		fprintf(stderr, "%s\n", commandLine.GetError().c_str());
		return 2;
	}
	if (commandLine.GetBool("help"))
	{
		printf("%s", commandLine.GetHelp().c_str());
		return 0;
	}
	if (commandLine.Has("bake-lod"))
	{
		if (commandLine.GetString("bake-lod").empty() || commandLine.Has("headless") || commandLine.Has("scene")
			|| commandLine.Has("output") || commandLine.Has("frames") || commandLine.Has("width") || commandLine.Has("height")
			|| commandLine.Has("lod") || commandLine.Has("gpu-scene"))
		{
			fprintf(stderr, "--bake-lod requires a local asset path and runs independently of rendering options\n");
			return 2;
		}
		std::string error;
		if (!KRenderApplication::BakeStaticMeshLOD(commandLine.GetString("bake-lod"), error))
		{
			fprintf(stderr, "%s\n", error.c_str());
			return 1;
		}
		return 0;
	}
	const auto& lod = commandLine.GetString("lod");
	if (lod != "auto" && lod != "0" && lod != "1" && lod != "2")
	{
		fprintf(stderr, "--lod must be auto, 0, 1 or 2\n");
		return 2;
	}
	KRenderApplication::SetStaticMeshOptions(lod == "auto" ? -1 : lod[0] - '0', commandLine.GetBool("gpu-scene"));
	const bool headless = commandLine.GetBool("headless");
	const int width = commandLine.GetInt("width"), height = commandLine.GetInt("height");
	const int frames = commandLine.GetInt("frames");
	std::string scenePath = commandLine.GetString("scene");
	std::string outputPath = commandLine.GetString("output"), lowerOutput;
	KStringUtil::Lower(outputPath, lowerOutput);
	if (width <= 0 || height <= 0 || frames <= 0 ||
		(!headless && (commandLine.Has("frames") || commandLine.Has("output"))) ||
		(commandLine.Has("scene") && scenePath.empty()) ||
		(commandLine.Has("output") && (lowerOutput.size() < 4 || lowerOutput.substr(lowerOutput.size() - 4) != ".png")))
	{
		fprintf(stderr, "--width, --height and --frames must be positive; --frames and --output require --headless; --scene requires a path and --output requires a .png path\n");
		return 2;
	}
	std::string pathError;
	if (!KRenderApplication::PreparePaths(scenePath, outputPath, pathError))
	{
		fprintf(stderr, "%s\n", pathError.c_str());
		return 1;
	}

	KEImGuiEditor editor;
	if (!editor.Init(headless ? RENDER_WINDOW_HEADLESS : RENDER_WINDOW_GLFW, width, height, scenePath.c_str()))
	{
		fprintf(stderr, "Failed to initialize KEditor or load scene: %s\n", scenePath.c_str());
		return 1;
	}
	bool success = editor.Run(frames, outputPath.c_str());
	editor.UnInit();
	return success ? 0 : 1;
}
