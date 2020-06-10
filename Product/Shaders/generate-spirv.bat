set DIR="%cd%"
echo DIR=%DIR%

for /R %DIR% %%f in (*.vert,*.frag) do ( 
echo %%f
glslc %%f --target-env=vulkan1.0 --target-spv=spv1.0 -o %%f.spv
)

::set SHADER_NAME=diffuse.vert
::glslc %SHADER_NAME% --target-env=vulkan1.0 --target-spv=spv1.0 -o ShaderCached/%SHADER_NAME%.spv
::set SHADER_NAME=diffuse.frag
::glslc %SHADER_NAME% --target-env=vulkan1.0 --target-spv=spv1.0 -o ShaderCached/%SHADER_NAME%.spv

pause