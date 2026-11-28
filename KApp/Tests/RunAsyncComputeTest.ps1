param(
    [string]$Scene,
    [switch]$SynchronousShaders,
    [switch]$Validation,
    [switch]$NegativeControl,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -version '[16.0,17.0)' -latest -property installationPath
$msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
$toolVersion = (Get-Content -LiteralPath (Join-Path $vsRoot 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
if (-not $SkipBuild)
{
    foreach ($project in @('KBase', 'KRender', 'KEngine', 'KApp'))
    {
        $projectFile = Join-Path $repoRoot "$project\$project.vcxproj"
        & $msbuild $projectFile /t:Build /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v142 "/p:VCToolsVersion=$toolVersion" /p:PreferredToolArchitecture=x64 /p:BuildProjectReferences=false /p:AsyncComputeTest=true /m:4 /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { throw "Build failed: $project" }
    }
}
if (-not $Scene)
{
    $Scene = (Get-Content -LiteralPath (Join-Path $repoRoot 'scene.txt') -Raw).Trim()
}
$binaryDir = Join-Path $repoRoot 'Product\Binaries\x64\Release'
$logDir = Join-Path $repoRoot 'KApp\x64\Release\AsyncComputeTest'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$caseName = if ($NegativeControl) { 'negative-control' } elseif ($SynchronousShaders) { 'sync-shaders' } else { 'async-shaders' }
$stdout = Join-Path $logDir "$caseName.stdout.log"
$stderr = Join-Path $logDir "$caseName.stderr.log"
$previousValidation = $env:KAPP_VULKAN_VALIDATION
$previousLayerEnables = $env:VK_LAYER_ENABLES
$savedSettings = @{}
foreach ($name in @('render_gui.json', 'imgui.ini'))
{
    $settingsPath = Join-Path $binaryDir $name
    if (Test-Path -LiteralPath $settingsPath)
    {
        $savedSettings[$settingsPath] = [System.IO.File]::ReadAllBytes($settingsPath)
    }
}
try
{
    if ($Validation)
    {
        $env:KAPP_VULKAN_VALIDATION = '1'
        $env:VK_LAYER_ENABLES = (@($previousLayerEnables, 'VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT', 'VALIDATION_CHECK_ENABLE_SYNCHRONIZATION_VALIDATION_QUEUE_SUBMIT') | Where-Object { $_ }) -join ','
    }
    $arguments = if ($NegativeControl) { '--negative-control' } else { '"' + $Scene + '"' }
    if ($SynchronousShaders) { $arguments += ' sync-shaders' }
    $test = Start-Process -FilePath (Join-Path $binaryDir 'KAsyncComputeTest.exe') -ArgumentList $arguments -WorkingDirectory $binaryDir -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    if (-not $test.WaitForExit(360000))
    {
        $test.Kill()
        throw "GPU regression timed out; inspect $stdout and $stderr"
    }
    Get-Content -LiteralPath $stdout | Select-String -Pattern '\[AsyncComputeTest\]'
    if ($NegativeControl)
    {
        if ($test.ExitCode -eq 0 -or -not (Select-String -LiteralPath $stderr -SimpleMatch 'Frame canary mismatch'))
        {
            throw "Negative control failed to detect the original cross-frame race; inspect $stdout and $stderr"
        }
        Write-Output 'Negative control PASS: original split queues reproduced the cross-frame mismatch.'
        return
    }
    if ($test.ExitCode -ne 0)
    {
        Get-Content -LiteralPath $stderr
        throw "GPU regression failed ($($test.ExitCode)); inspect $stdout and $stderr"
    }
    $errors = Select-String -LiteralPath $stdout -Pattern '\[Vulkan\].*\[Error\]' -ErrorAction SilentlyContinue
    if ($errors) { throw "Vulkan validation errors reported; inspect $stdout" }
}
finally
{
    $env:KAPP_VULKAN_VALIDATION = $previousValidation
    $env:VK_LAYER_ENABLES = $previousLayerEnables
    foreach ($settingsPath in $savedSettings.Keys)
    {
        [System.IO.File]::WriteAllBytes($settingsPath, $savedSettings[$settingsPath])
    }
}
