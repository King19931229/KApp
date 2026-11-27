# Circular DOF 第二阶段验证记录

基线：`7518c637673bc06674b892d39ad7254fb7917aa6`。

阶段 A/B/C 的实现与验证已完成，并与 ImGui 和并行 TileMAX 扩展合并为一个 DOF 提交。当前状态见“ImGui 与并行 TileMAX 补充交付”，该节取代前期 Tile 后置的状态。

## 阶段 A：光学尺度

- 移除相对 `CoCMin/CoCMax` 的归一化，统一输出 Near 为负、Far 为正的像素半径和线性权重。
- 相机 `GetFov()` 为弧度制垂直 FOV，右手视空间深度使用 `-viewZ`。焦距、对焦距离、深度和等效成像面使用同一场景长度单位。
- CPU 上传两个 `vec4`：`dofParams = (FocusDistance, OpticalDiameterScale, FullPixelRadiusScale, WorkPixelScale)`；当前 `dofParams2 = (ValidAndEnabled, DebugView, TileActive, Reserved)`，后续 Tile 扩展使用第三个分量，不扩大参数布局。半径换算中的 `0.5` 仅表示直径转半径。
- 只接受标准透视、有效光学参数和一致的 X/Y 像素尺度；非法状态继续运行当前帧的清晰回退，恢复有效参数后自动恢复 DOF。
- 仓库搜索确认旧光学参数只用于 DOF 与 ImGui 控件；`render_gui.json` 只保存菜单开关，没有待迁移的旧光学字段。

已执行：

- `python -B KApp/Tests/KDofReferenceTests.py`：5424 项检查通过，含几何公式、FStop 倍率、焦距响应、符号及非法输入。
- `python -B KApp/Tests/KDofShaderTests.py`：7 个实际 Shader 变体编译并通过 SPIR-V 校验。
- x64 Debug：依次构建 KRender、KEngine、KApp 成功。

构建环境限制：直接构建解决方案会触发 metis、glslang、astc-encoder 的旧 CMake ZERO_CHECK 路径错误。验证采用依次构建项目、复用已准备好的第三方库，未修改依赖工程。

上述 CPU/Shader 检查不能代替实际 GPU 画质、生命周期或同步验证；这些结果在后续阶段单独记录。

补充实际应用验证：x64 Debug 的 Bonza 场景以 65×37 运行 3 帧并保存 PNG，正常退出；启用 Vulkan validation，未出现 VUID 或同步错误。该尺寸的应用截图包含 UI，只用于渲染链冒烟检查，画质结论使用下面的独立数值夹具。

## 阶段 B：合成与 Ghosting

- 保留 `nearAlpha = clamp(N.a, 0, 1)`，不修改卷积 Alpha，不增加 MAX 覆盖率实验。Far 继续使用目标全分辨率像素的原始 farCoC，先 Far 后 Near。
- 所有层检查原始 RGBA、有效分母和归一化颜色；无效 Far 回 SceneColor，无效 Near 透明。非有限源色及权重一起排除；非有限 SceneColor 使用黑色回退。
- 实际 GPU 注入发现：`F.a = FP16(1e-5)` 刚超过 EPSILON 时，有限的恢复颜色仍可能使最终混合超出 FP16 最大有限值。现额外检查实际混合结果是否可存入 RGBA16F；超范围拒绝该层，正常 HDR 不限幅。诊断 NearResult 无法存入 FP16 时显示洋红色。
- 10 个输出模式复用 Final：最终结果、光学直径、全分辨率半径、工作半径、有符号 CoC、原始 NearCoC、原始 N.a、NearAlpha、NearResult、Base。带单位的有符号显示使用 `abs(v)/(1+abs(v))` 编码，负蓝正红；该编码不进入实际 CoC 或合成。

已执行：

- CPU 参考：5461 项检查通过；7 个 Shader 变体编译及 SPIR-V 校验通过。
- x64 Debug 的 KRender、KEngine、KApp 与 GPU 夹具构建成功。
- NVIDIA GeForce RTX 4070 Laptop GPU：45 个实际 GPU 用例通过 CPU 对照。覆盖光学倍率、全焦、HDR、非法光学参数、正交/缺失相机、宽高比不匹配、全部诊断模式、Reload、UnInit/Init，以及直接注入 Composite 的负/微小/超一 Alpha 和 NaN/Inf。
- 所有正常输出的最大 RGB 对照误差不超过 0.00390625；检查同时包含 FP16 水平中间量和垂直输出量化。全焦输出逐像素误差为零。
- GPU 夹具启用 Vulkan Synchronization Validation，最终运行无 VUID、同步或图像布局错误。早期夹具的非法 TransferDst 上传和静态描述符布局问题已在测试夹具内纠正，没有更改生产 RHI 或 RT 用途。

薄近景诊断（32×20，像素 16,10，线性 RGB）：原始 NearCoC=1，NearAlpha=0.12939453125，NearResult=(2.0625,2.0625,0.5)，Base=(0.125,4,0.5)，Final=(0.375732421875,3.748046875,0.5)。Base 与原清晰前景一致，说明该用例的前景纹理残留来自剩余 Base 贡献。N.a 小于原始 NearCoC 符合保留的覆盖率基线，不能据此判定覆盖率错误；单层颜色/深度仍缺失真实遮挡背景。

诊断图从左到右为 SceneColor、NearResult、Base、Final，原始数值和 GPU 日志位于 `.codex-diagnostics/dof-phase2/stage-b-gpu`。本阶段未实施隐藏背景重建，因此不宣称物理正确遮挡或完全消除 Ghosting。

## 阶段 C：两档分辨率与最终验证

- `Init(width,height,ratio)` 接受 0.5/0.25，内部由整数因子 2/4 驱动；非支持比例和 NaN/Inf 比例告警后回退 0.5。默认调用仍为 0.5，光学初值保持不变。
- 工作尺寸向上取整。预处理逐源像素分类、预乘，仅按有效坐标数平均；异常源色贡献为零但有效坐标仍计入 footprint 面积。
- `dofParams.w` 为整数因子的倒数，独立于取整后的工作尺寸。上采样按实际 footprint 中心定位，对末块中心做局部插值修正，单 texel 的工作轴直接采样该 texel。
- 零尺寸 Init 在分配前失败；零尺寸 Resize 返回 false 并保留上一次有效资源，后续有效 Resize 可正常恢复。

已执行并通过：

- x64 Debug/Release 均依次构建 KRender、KEngine、KApp 和 KDofGpuTests；最后一次 7 个实际 Shader 变体编译及 SPIR-V 校验通过，CPU 参考 5461 项通过。
- Debug：74 个 GPU 用例。Release：同组 74 个用例及 2 个 1080p 计时/常量 HDR 用例，共 76 个。
- 包含两档 33×21、1×1、1×5、5×1、3×3、65×37，35×35 impulse response，全焦、连续尺寸变化、Reload、UnInit/Init、零尺寸拒绝及不支持比例回退；另覆盖非对称/非有限投影和非有限/负深度。
- 两档都用注入的工作纹理坐标斜坡单独验证 Composite 对齐。全部 GPU 卷积和合成对照通过；最终颜色最大绝对误差 0.00390625，全焦及 1080p 常量 HDR 样点误差为零。
- 对照阈值为 `0.004 + 0.015 * abs(reference)`，适用于 FP16 中间量及纹理插值。卷积和合成独立检查，结果 JSON 记录每个用例的实际最大误差。
- Debug/Release GPU 夹具均启用 Vulkan Synchronization Validation，最终日志无 VUID、同步或图像布局错误。
- 最终 Release 应用以 641×359 运行默认启动场景 3 帧并保存截图，正常退出，无 VUID、同步、布局或非法 DOF 参数日志。截图含现有 UI，仅作为完整渲染链冒烟验证。

1080p 实测（RTX 4070 Laptop GPU，Release，验证层开启，常量 HDR 全近景夹具；GPU timestamp 测量完整六 Pass，预热一轮后平均十一轮）：

| 工作比例 | 工作尺寸 | 六张工作 RT + Final 理论像素存储 | 七张 VkImage 对齐后内存需求 | DOF GPU 时间 |
|---|---|---:|---:|---:|
| 0.5 | 960×540 | 39.55 MiB | 45.00 MiB | 0.836893 ms |
| 0.25 | 480×270 | 21.75 MiB | 25.3125 MiB | 0.736666 ms |

本次测量的图像需求减少 43.75%，GPU 时间减少约 11.98%。内存值来自各 VkImage 的 `vkGetImageMemoryRequirements`，不包含 allocator 页、其他渲染资源、驱动或实际驻留开销；GPU 时间仅代表该夹具，不代表完整场景帧时。

同一 65×37 混合夹具两档最终线性 RGB 的平均绝对差为 0.19555，最大差为 0.466797。固定 ±8 工作 texel 在两档分别约为 ±16/±32 全分辨率像素，权重归一化参考也随之改变；两档不保证相同画质。默认继续使用 0.5。

最终原始读回、对照 JSON、诊断图和 CSV 位于 `.codex-diagnostics/dof-phase2/stage-c-debug-gpu` 与 `stage-c-release-gpu`；应用截图为 `.codex-diagnostics/dof-phase2/stage-c-scene-641x359.png`。

未执行：硬件带宽计数器、allocator/驱动驻留显存测量、真实复杂场景两档同相机的长时间性能对照及手动 GUI 拖窗操作。自动夹具已覆盖 GBuffer/DOF 连续 Resize、Reload 与重初始化。阶段 C 完成时 Tile 后置；后续实现及验证见下节，NearAlpha MAX 实验与背景重建仍后置。

## ImGui 与并行 TileMAX 补充交付

重新逐项核对用户提供的原始 `DOF_Phase2_Repair_Plan_Revised.md` 第 5～7 节，保留其并行规约与消费约束。这里的 TileMAX 和 NearAlpha MAX 覆盖率实验是两个独立功能。

ImGui：`Settings → DOF` 中提供实际生效的 `Enable`、`Working resolution`（`1/2`、`1/4`）、`Near TileMAX` 和 `View → Near TileCoC`，显示实际工作尺寸。工作比例仍默认 1/2；Tile 默认关闭。格式不支持时说明不可用；并非提供无实现的开关。DOF 总开关关闭时跳过预处理和卷积，以当前 SceneColor 更新 Final。

原文要求与实现：

| 原始 MD | 当前实现 |
|---|---|
| §5.2：8×8、64 线程、shared memory MAX | `near_tile_max.comp` 每线程对应一个工作 texel，先计算自身 2×2/4×4 源 footprint 的 MAX，再以 32→16→8→4→2→1 做全组并行树形规约。每轮 barrier，所有越界线程贡献零且参与同步；第 0 线程仅负责最终写入。没有单线程扫描整块 Tile 的生产实现。 |
| §5.2：原始 Depth、共享 CoC、R16F | 与 Preprocess 使用相同的深度和 CoC 函数，输出工作尺寸向上取整后再按 8 向上取整的 R16F Storage Image。检测已启用的 extended storage formats 特性及 R16F sampled/storage 支持。 |
| §5.2：极小值与 FP16 | 输出为源 Near CoC MAX 的 FP16 编码；极小正值至少编码为 2^-14，避免被量化或次正规数清零误判为空。它只保存非空证据，不覆盖源 CoC、预乘权重或覆盖率，也不是实际像素半径。 |
| §5.3：固定核保守消费 | H/V 共用允许的 3×3 邻近 Tile 查询，覆盖 H 的横向源范围和 V 的原始二维源范围；只剔除确定全零的区域，负值或 NaN/Inf 保留卷积。空区明确写零，不 discard。非空区域仍运行原始 17-tap 复数卷积，Far 不受影响。 |
| §5.4：调度与同步 | `Preprocess → TileMAX（可选）→ Near H/V → Far H/V → Composite`。Depth 可供 Compute/Fragment 读取；Tile 采用 `GENERAL→GENERAL` 内存屏障连接上帧 Fragment 读取、当帧 Compute 写入和 H/V/Composite 读取，以匹配 KApp 现有 Storage Image 采样描述符约定。 |
| §6：两档分辨率 | ImGui 请求由帧边界 `ApplySettings` 应用，整数因子、源 footprint、Tile footprint 和 Composite 格点约定一致；切档保留 Final 的原生图像及下游绑定。 |
| §7：资源 | 常驻六张 RGBA16F 工作 RT 和独立 Final，首次启用 Tile 才分配一张低分辨率 R16F；关闭时保留已分配 Tile 供再次启用，未引入 Dilation RT/Pass 或全分辨率 CoC。 |

复查时移除了最初 UI 切档路径新增的全局 flush/Wait。工作比例切换采用现有帧完成 Fence 延迟销毁旧工作 RT、Pass 和 Pipeline；Fence Pending 时保留旧对象，Signaled 后非阻塞回收。另外发现原有 RT 初始化辅助函数会为布局转换和 Storage 清零调用单次提交及 `vkWaitForFences`，因此为 Color/Storage 创建接口增加默认关闭的 `deferredInitialization` 选项，仅 DOF 启用：分配后保持 UNDEFINED，由正常帧命令完成初始布局转换和完整写入。其他调用保留原默认行为。Tile 开关、Tile Pass 及质量切换均不新增 CPU/GPU 等待。Final 和其 DebugDrawer 在仅切工作比例时保持稳定。切档期间新旧工作对象可能短暂重叠；这是飞行帧资源退役，非第二套常驻 Near/Far 中间纹理，稳态资源表和下面的计时内存值不包含该瞬时峰值。

原有 Graphics Pipeline Handle 的销毁还会无条件 `DeviceWaitIdle`。退役组只在最后使用 Fence 完成后，为其独占 RenderPass 的 Pipeline 调用 `UnInit(resourcesComplete=true)`，提前安全释放原生 Handle，避免引用销毁再触发设备等待；其他调用默认仍保留原同步行为。这个选项的前提是全部共享 Handle 的 GPU 使用均已完成，不可对仍在使用的通用 Pipeline 任意设置。

延迟初始化复查中还修正了“切档重建 Tile 后当帧关闭 Tile/DOF”的布局错误：Composite 描述符仍声明 GENERAL，因此已分配 Tile 的首次 UNDEFINED→GENERAL 转换在开关分支外执行；首次 MAX 内容写入仍按开关调度。

最终实际验证（2026-11-27）：

- x64 Debug/Release 均依次构建 KRender、KEngine、KApp、KDofGpuTests 成功。
- CPU 参考 5461 项通过。10 个实际 Shader 变体编译及 SPIR-V 校验通过；另检查编译后的 Tile Compute 具有 8×8 线程组、共享 `float[64]` 和 WorkGroup 内存 barrier。
- Debug 111 个 GPU 用例通过；Release 为同组 111 个数值用例加 12 个 1080p 计时样例，共 123 个通过。两次运行各追加 8 帧真实渲染器 UI 回调切换，Final 的原生 VkImage 始终稳定。五个显式切档用例还验证 Pending Fence 保留旧 RT、真实 Fence 完成后回收旧 RT。
- 两档均将实际 R16F Tile 对照独立 CPU 源 MAX 参考，包含末块、全零、单源高值、跨 Tile 边界、极小正权重、Reload 和重初始化；最大 Tile 编码误差为零。所有 Tile 输出有限且在 [0,1]。
- 两档 65×37 的混合、跨 Tile 脉冲、全远、全近、全焦及极小 CoC 样例中，Tile 开/关的 Near/Far/Final 逐分量差值均为零。总体 GPU/CPU 最终颜色最大绝对误差仍为 0.00390625。Composite 基线及上述 Ghosting 诊断仍成立，Tile 不修改覆盖率。
- Debug/Release 最终运行均启用 Vulkan Synchronization Validation，无 VUID、同步、布局或生命周期错误。原始读回、CSV、JSON、诊断 PNG 和日志位于 `.codex-diagnostics/dof-phase2/ui-debug-gpu` 与 `ui-release-gpu`。

当前版本 1080p 实测（RTX 4070 Laptop GPU，Release，验证层开启；完整 DOF 六 Pass，开启 Tile 后七 Pass；预热一轮、平均十一轮）：

| 工作比例 | 输入样例 | Tile 关闭 GPU ms | Tile 开启 GPU ms |
|---|---|---:|---:|
| 1/2 | 全近景恒定 HDR | 1.24947 | 1.40184 |
| 1/2 | 全远景 | 1.75299 | 1.30819 |
| 1/2 | 稀疏近景 | 1.28455 | 1.84193 |
| 1/4 | 全近景恒定 HDR | 1.20483 | 1.36749 |
| 1/4 | 全远景 | 1.31102 | 1.35574 |
| 1/4 | 稀疏近景 | 1.05448 | 1.50503 |

本次仅 1/2 全远景样例获益，其余样例增加开销；重复深度/CoC 计算、邻域查询及新增调度均有成本，不能由此宣称 Tile 普遍更快。遵照原文 §5.3 保留开关并默认关闭，不新增第二张 Tile/Dilation 资源。以上是当前版本的单次测量；前面的阶段 C 时间是此前构建的历史基线，不混作同次性能对照，也不代表复杂场景帧时。

本轮 CSV 同时记录已分配 Tile 的尺寸，关闭但保留的图像也计入内存：

| 分配状态 | Tile 尺寸 | 全部 DOF 图像理论存储 MiB | VkImage 对齐后内存需求 MiB |
|---|---|---:|---:|
| 1/2，Tile 已分配 | 120×68 | 39.56635 | 45.03125 |
| 1/4，Tile 未分配 | 无 | 21.75293 | 25.31250 |
| 1/4，Tile 已分配 | 60×34 | 21.75682 | 25.32031 |

图像内存需求不等于实际驻留显存，也不包含切档期间旧资源退役的瞬时峰值。未执行：当前面板的手动鼠标点击和截图验收、真实复杂场景长时间统计、硬件带宽计数器、allocator/驱动驻留显存及切档峰值测量。实际验证的是生产控件同一请求字段、真正 Renderer 更新路径与 GPU 输出；未执行项目不记为通过。

## 修改位置与复查入口

| 文件及入口 | 变更 |
|---|---|
| `KRender/Internal/Object/KDepthOfField.cpp`：`Init`、`ApplySettings`、`Resize`、`Execute` | 比例选择、资源重建与退役、相机/光学验证、参数上传与并行 Tile 调度 |
| `KRender/Internal/Object/KDepthOfField.h`：`DebugView`、`RetiredWorkingResources` | 诊断枚举、光学/ImGui 接口、整数因子、RT 生命周期及完成 Fence |
| `KRender/Publish/KCamera.h:200` | 只读投影模式接口 |
| `KRender/Internal/KRenderImGui.cpp:247` | 诊断视图与有效光学控件，旧控件移除 |
| `Product/Shaders/dof/common.h:34`、`:62` | 共享 signed pixel CoC 与格点对齐 |
| `Product/Shaders/dof/preprocess.frag:19` | 逐源像素预乘、有限值保护和有效 footprint 平均 |
| `Product/Shaders/dof/composite.frag:19`、`:106` | 原始分母恢复、覆盖率基线、格式回退和诊断输出 |
| `Product/Shaders/dof/near_tile_max.comp:35`、`tile_config.h`、`tile.h` | 8×8 并行树形 MAX、共享 Tile 尺寸、固定核保守邻域查询 |
| `Product/Shaders/dof/horizontal.frag`、`vertical.frag` | Near Tile 变体的空区写零分支 |
| `KRender/Internal/Render/KRenderer.cpp`：`Render`、`Update` | 沿用实际帧完成 Fence，帧边界应用 ImGui 请求 |
| `KRender/Interface/IKRenderTarget.h`、`Internal/Vulkan/KVulkanRenderTarget.*`、`KVulkanFrameBuffer.*` | 可选延迟初始化，避免 DOF RT 创建的单次提交等待 |
| `KRender/Interface/IKPipeline.h`、`Internal/KPipelineBase.*`、`Vulkan/KVulkanPipeline.*` | 已完成 Fence 的独占 Pipeline 组安全释放，保留其他调用原默认同步行为 |
| `KRender/Interface/IKRenderDevice.h`、`Internal/Vulkan/KVulkanRenderDevice.cpp` | R16F Storage/Sampled 和启用特性的能力检查 |
| `KApp/Tests/KDofReferenceTests.py`、`KDofShaderTests.py` | CPU 数学参考和实际 Shader/SPIR-V 校验 |
| `KApp/Tests/KDofGpuTests.cpp/.py/.vcxproj`、`KDofUpload.comp` | 实际渲染 Pass 集成测试、独立 Composite 注入、合法测试数据上传和计时 |

`circular.h` 与审查基线无差异，版权、系数、生成器来源保持原样。H/V 仅增加 Near Tile 变体的空区写零分支；原有复数乘法、17-tap 步长和打包不变。按 A → B → C 完成实现与验证，最终与 ImGui/Tile 扩展统一交付；具体命令见同目录 README。

## 资源与算法边界

默认保持 `Preprocess → Near H → Near V → Far H → Far V → Composite`；开启 Tile 时在 Preprocess 后追加并行 TileMAX。Near/Far 各自包含预乘 RGB 和有效权重；颜色恢复使用原始卷积 Alpha。

六张 RGBA16F 工作纹理保持原布局：Near H 使用前两张复数中间纹理，Near V 覆盖 NearColor；随后 Far H 复用全部四张，Far V 覆盖 FarColor。Final 独立保存全分辨率结果。

Final 复用评估：当前下游 PostProcessResult 读取 DOF Final 并写入 Deferred Final，直接别名会造成同 Pass 读写同一资源；后处理管理器也没有可直接借用的 Ping-Pong 接口。本轮保留独立 Final。

Kernel 系数及其生成器版权和来源保持不变。CoC 映射、Near 覆盖率、安全回退及 Tile 空区剔除属于 KApp 工程方案，不声称是 Garcia 商业 Shader 的精确实现。NearAlpha MAX 覆盖率实验及背景重建后置；并行 TileMAX 仅作为可关闭的空区剔除优化。
