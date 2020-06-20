set DIR="%cd%"
echo DIR=%DIR%

for /R %DIR% %%f in (*.vert,*.frag) do ( 
glslc %%f -std=450 --target-env=vulkan1.0 --target-spv=spv1.0 -o %%f.spv
del %%f.spv
)

REM set SHADER_NAME=diffuse.vert
REM glslc %SHADER_NAME% -std=450 --target-env=vulkan1.0 --target-spv=spv1.0 -o ShaderCached/%SHADER_NAME%.spv
REM set SHADER_NAME=diffuse.frag
REM glslc %SHADER_NAME% -std=450 --target-env=vulkan1.0 --target-spv=spv1.0 -o ShaderCached/%SHADER_NAME%.spv

pause