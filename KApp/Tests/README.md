Run these from the repository root in a Visual Studio 2019 developer shell after preparing the main project's dependencies:

```powershell
msbuild KApp/Tests/KPixelFormatTests.vcxproj /p:Configuration=Debug /p:Platform=x64
& KApp/Tests/bin/Debug/KPixelFormatTests.exe
msbuild KApp/Tests/KPixelFormatTests.vcxproj /p:Configuration=Release /p:Platform=x64
& KApp/Tests/bin/Release/KPixelFormatTests.exe
```

The standalone executable tests pixel sizes, compression blocks, mip chains, volume/cube/array sizes, overflow rejection, format names, saved resource values, DDS hardware passthrough and software decoding, ASTC KTX1 formats and sRGB cubemap mip decoding, and saving RGBA8 sRGB images. It generates DDS/KTX samples in memory and removes its temporary PNG after a successful save.

Run the DOF circular-convolution reference checks with:

```powershell
python KApp/Tests/KDofReferenceTests.py
```

DOF 第二阶段另外提供实际 Shader 编译检查和 GPU 集成夹具。在 Visual Studio 2019 x64 developer shell 中，先依次构建已有依赖齐备的项目：

```powershell
msbuild KRender/KRender.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:PreferredToolArchitecture=x64
msbuild KEngine/KEngine.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:PreferredToolArchitecture=x64
msbuild KApp/KApp.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:PreferredToolArchitecture=x64
msbuild KApp/Tests/KDofGpuTests.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:PreferredToolArchitecture=x64
python -B KApp/Tests/KDofShaderTests.py
python -B KApp/Tests/KDofGpuTests.py --configuration Debug
```

将 Configuration 改为 Release 可重复 Release 构建和 GPU 测试。Shader 检查需要 PATH 中存在 Vulkan SDK 的 `glslangValidator`，存在 `spirv-val` 时也会运行 SPIR-V 校验。

GPU 夹具加载实际 `KDepthOfField` 和 Shader，使用合成 RGBA16F 颜色/线性深度驱动全部 Pass，读回 Near/Far/Final 对照 CPU 参考；异常合成用例另向实际 Composite 注入负 Alpha、NaN/Inf 等输入。夹具通过 Storage Image 上传数据，不改变生产 RT 的用途标志。

默认运行完整两档分辨率、奇数尺寸和生命周期测试；`--phase b` 只运行阶段 B 用例。当前完整集另外覆盖并行 TileMAX 源 CoC MAX、Tile 开关结果一致性、边界脉冲、极小正权重、Tile 诊断及实时工作比例切换。切档夹具显式验证 Pending Fence 保留旧 RT、Fence 完成后释放；最后通过真实渲染器的 UI 回调连续切换比例、Tile 和总开关。`--profile` 额外运行 1080p 两档全近景、全远景及稀疏近景的 Tile 开/关 GPU timestamp 测量，预热一轮后平均十一轮。测试用等待和读回只发生在独立测试进程；生产切档通过现有帧 Fence 非阻塞退役旧资源。`--verify-only` 可以重新检查已有读回文件，避免重复初始化引擎。

ImGui 使用 `Settings → DOF → Working resolution / Near TileMAX`；Tile 默认关闭。Shader 检查会编译实际 Tile Compute 和 Near H/V 变体，并检查 Tile SPIR-V 的 8×8 线程组、共享 `float[64]` 和 WorkGroup barrier。

输出位于 `.codex-diagnostics/dof-phase2/gpu`（可用 `--output-dir` 修改）：原始 RGBA16F、GPU 时间与图像内存需求 CSV、CPU 对照 JSON、Vulkan 日志和诊断 PNG。`ghosting-comparison.png` 从左到右为 SceneColor、NearResult、Base、Final；PNG 统一使用 Reinhard + gamma 显示，数值比较使用未经色调映射的 FP16 读回。

GPU 卷积对照会模拟水平中间 RT 和垂直输出的 FP16 量化；合成再独立使用实际读回的 N/F 验证，避免微小分母放大量化差异而混淆故障阶段。Tile 使用独立 CPU MAX 参考并读回实际 R16F；额外比较同一输入的 Tile 开/关 Near/Far/Final。CSV 记录实际分配的 Tile 尺寸，即使关闭后保留该图像也纳入理论存储与实际图像需求。内存数据是各 VkImage 的对齐后需求总和，不包含切档退役中的旧资源峰值、allocator 页、其他渲染资源或驱动开销。

设计边界、各阶段实际验证结果和尚未执行的项目见 [DOFPhase2Validation.md](DOFPhase2Validation.md)。

---

# Async compute GPU regression

Run from PowerShell with Visual Studio 2019/v142, the Vulkan SDK and a GPU exposing a separate compute family:

```powershell
.\KApp\Tests\RunAsyncComputeTest.ps1 -Validation
.\KApp\Tests\RunAsyncComputeTest.ps1 -SkipBuild -Validation -SynchronousShaders
.\KApp\Tests\RunAsyncComputeTest.ps1 -SkipBuild -NegativeControl
```

The runner builds a separate `KAsyncComputeTest.exe`; the normal KApp entry remains unchanged. Pass `-Scene <path>` to override the scene named in the repository's `scene.txt`.

The test checks 1,000 frame IDs and uniform-buffer checksums on real graphics/compute queues with two frames in flight. It then renders more than 1,000 scene frames while exercising AsyncCompute/RTAO/SSR switches, both RHI modes, camera/object motion, resize, minimize/restore, a secondary window and compute fallback. Validation includes submission-time synchronization checks on SDK versions that support the setting.

The negative control restores the old split graphics/present submission mapping. It succeeds only when the GPU canary detects a wrong frame value. GPU timestamp overlap is reported as an observation, not a performance threshold.

Per-case logs are saved under `KApp/x64/Release/AsyncComputeTest`. Existing GUI settings and validation environment variables are restored after each run.

For the normal application, `KAPP_VULKAN_VALIDATION=1` enables the existing validation layer and synchronization validation. If unset, the existing `USE_VALIDAION_LAYER` macro determines whether validation is enabled.


# Fixed-width interface regression

Build KApp/Tests/KSizeTypeTests.vcxproj with /p:Platform=x64 or /p:Platform=Win32, and /p:Configuration=Debug or Release. Run KApp/Tests/bin/<Platform>/<Configuration>/KSizeTypeTests.exe.

This standalone test needs no third-party binary libraries. It checks public interface widths, rejected Buffer capacity overflow, image byte calculations above 4 GiB, host-size limits and negative picking coordinates. Pixel-format/codec tests remain in KPixelFormatTests.vcxproj.

# GPUScene shadows and shared LOD regression

Run from the repository root with Visual Studio 2019/v142, the Vulkan SDK and a Vulkan GPU:

```powershell
.\KApp\Tests\RunShadowLODTests.ps1 -Validation
.\KApp\Tests\RunShadowLODTests.ps1 -SkipBuild -Validation
```

The runner builds `KShadowLODTests.exe` and generates a temporary curved OBJ with distinct LOD0/1/2 geometry. It checks the submitted CPU draws and reads back GPUScene indirect arguments for every shadow cascade, compares rendered shadow depth against the CPU path, and exercises forced/automatic LOD, hysteresis, static-cache invalidation, offscreen casters, missing-LOD fallback, Alpha Mask updates, receiver clipping, immediate/deferred RHI and single/multithreaded submission. Main and shadow geometry must share the main view's resolved LOD.

Logs are saved under `KApp/Tests/bin/ShadowLOD`. Temporary fixture files are removed and existing GUI settings and validation environment variables are restored after each run.

# SVO GI GPU regression

Run the independent SVO fixture with Visual Studio 2019/v142, the Vulkan SDK and a Vulkan GPU:

```powershell
.\KApp\Tests\RunSvoTests.ps1 -Configuration Release -Validation
.\KApp\Tests\RunSvoTests.ps1 -Configuration Debug -Validation
.\KApp\Tests\RunSvoTests.ps1 -Configuration Release -SkipBuild -Validation
```

The runner compiles the production SVO shaders and fixture shaders, validates their SPIR-V, and runs `KSvoGpuTests.exe`. The fixture exercises the actual Octree queries and builders, including data index zero, empty slots, duplicate fragments, empty trees and repeated rebuilds. It compares every face and mip against a CPU reference with UNORM quantization after each level. Deferred RHI records two entire mip chains before submission, and checks one dispatch per level.

Both voxel paths and RHI modes are covered, with bounce disabled/enabled, scene changes, full clearing of dense attributes and radiance, and lifecycle checks. The Octree UBO case reads GPU parameters before submitting the caller's deferred batch. GPU uploads/readbacks and waits in the fixture are test operations; the production Octree path retains its existing synchronous rebuild design.

Logs and numerical summaries are saved under `.codex-diagnostics/svo/<Configuration>`. The runner restores GUI settings and its validation environment variable after execution.
