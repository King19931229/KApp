# Clipmap GI regression

Run from the repository root with Visual Studio 2019/v142, the Vulkan SDK and a Vulkan GPU:

```powershell
python -B KApp/Tests/KClipmapShaderTests.py
.\KApp\Tests\RunClipmapTests.ps1 -Validation -Configuration Release
.\KApp\Tests\RunClipmapTests.ps1 -Validation -Configuration Debug
```

Use `-SkipBuild` after building the same configuration. The runner creates an independent `KClipmapTests.exe`, hides its window, restores existing GUI settings and removes its temporary scene. Logs are written to `KApp/Tests/bin/Clipmap/<Configuration>`.

Shader compilation uses a temporary directory and removes generated sources and SPIR-V automatically. Pass `--output-dir <path>` to retain those artifacts when investigating a failure.

For a production-scene camera path, add `-Scene <absolute-scene-or-asset-path>`. This mode uses the normal `64³ × 7` clipmap, warms up 32 frames, pans 512 world units over 64 frames, returns over another 64 frames, then settles for 64 frames. It captures PNGs every four moving frames without the debug UI or depth of field, and reports the same-pose GI difference after returning. Direct assets use an interior camera at 16% of the asset height. Results are saved under the configuration's `production-<name>` subdirectory. This is a visual smoke test, separate from the synthetic numerical assertions.

```powershell
.\KApp\Tests\RunClipmapTests.ps1 -SkipBuild -Validation -Scene D:\KApp\Product\Models\scenes\sponza\Sponza01.gltf
```

The CPU tests exercise the production update planner, including immediate initialization of all seven levels, deferred camera reversal, eventual completion of dependent updates, negative coordinates and randomized multi-axis/whole-volume movement.

The GPU tests execute the production shaders. They compare all six directional downsample results with an independent front-to-back reference, check negative odd clipmap bounds and restore native coarse visibility after fine coverage moves. Fixed-source propagation is repeated to check source immutability, determinism, HDR preservation and matching energy at different levels.

The production hard/soft shadow sampling helpers are rendered with receivers inside and outside all six UV/depth bounds. Out-of-coverage receivers must remain unshadowed, while covered lit and occluded receivers retain their expected comparison result. This guards the distant-model blackout when a receiver leaves CSM coverage.

A temporary scene includes a plane, an inclined triangle, a degenerate face and more than 1,024 overlapping faces. Tests cover first-frame occupancy at all seven levels, saturated hit counts, immediate/deferred RHI, camera reversal, incremental/full-rebuild occupancy, Reload and Resize.

Screen tests check pure-indirect output, HDR values above one, black and sky pixels, non-finite normals, odd dimensions and a one-pixel target. A 161-frame synthetic camera path moves the real screen pass across snapped clipmap bounds with deliberately different radiance at each level. Constant opaque radiance is also checked at six distances. A separate six-face HDR field checks that all interleave phases integrate the cones with the same weights.

These tests use small synthetic scenes and numeric readback. They do not establish visual equivalence for every production scene or a performance budget. The existing GI gain remains unchanged; removal of duplicate direct light and early tone mapping intentionally changes the old exposure. CPU/GPU waits and readbacks occur only in the test executable.

Three-dimensional volumes are exported through a fixture compute shader to a storage buffer; the existing 2D-only framebuffer readback path is not extended. Production rendering keeps the original public interfaces, time slicing and interleaved cone tracing. The additional RGBA16F source volume plus promotion of the final volume requires about 138 MiB more memory at the default `64³ × 7` configuration; screen targets are additional.
