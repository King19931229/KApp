param([switch]$SkipBuild, [switch]$Validation, [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$logDir = Join-Path $repoRoot ('.codex-diagnostics\svo\' + $Configuration)
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$summary = Join-Path $logDir 'summary.json'
if (Test-Path -LiteralPath $summary) { Remove-Item -LiteralPath $summary }
& python -B (Join-Path $PSScriptRoot 'KSvoShaderTests.py') --output-dir (Join-Path $logDir 'shaders')
if ($LASTEXITCODE -ne 0) { throw 'SVO Shader/SPIR-V checks failed' }
if (-not $SkipBuild)
{
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vsRoot = & $vswhere -version '[16.0,17.0)' -latest -property installationPath
    if (-not $vsRoot) { throw 'Visual Studio 2019/v142 is required' }
    $msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
    $toolVersion = (Get-Content -LiteralPath (Join-Path $vsRoot 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
    foreach ($project in @('KBase\KBase.vcxproj', 'KRender\KRender.vcxproj', 'KEngine\KEngine.vcxproj', 'KApp\Tests\KSvoGpuTests.vcxproj'))
    {
        & $msbuild (Join-Path $repoRoot $project) /t:Build "/p:Configuration=$Configuration" /p:Platform=x64 /p:PlatformToolset=v142 "/p:VCToolsVersion=$toolVersion" /p:PreferredToolArchitecture=x64 /p:BuildProjectReferences=false /m:4 /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { throw "Build failed: $project" }
    }
}
$binaryDir = Join-Path $repoRoot ('Product\Binaries\x64\' + $Configuration)
$stdout = Join-Path $logDir 'stdout.log'
$stderr = Join-Path $logDir 'stderr.log'
$fixture = Join-Path $logDir 'svo-fixture.obj'
$previousValidation = $env:KAPP_VULKAN_VALIDATION
$savedSettings = @{}
$missingSettings = @()
foreach ($name in @('render_gui.json', 'imgui.ini'))
{
    $path = Join-Path $binaryDir $name
    if (Test-Path -LiteralPath $path) { $savedSettings[$path] = [System.IO.File]::ReadAllBytes($path) }
    else { $missingSettings += $path }
}
try
{
    $env:KAPP_VULKAN_VALIDATION = if ($Validation) { '1' } else { '0' }
    $test = Start-Process -FilePath (Join-Path $binaryDir 'KSvoGpuTests.exe') -ArgumentList ('"' + $fixture + '"') -WorkingDirectory $binaryDir -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    while (-not $test.WaitForExit(1000))
    {
        if ((Get-Date) - $test.StartTime -gt [TimeSpan]::FromMinutes(10))
        {
            $test.Kill()
            $test.WaitForExit()
            throw "SVO GPU regression timed out; inspect $stdout and $stderr"
        }
    }
    $test.Refresh()
    Get-Content -LiteralPath $stdout | Select-String -Pattern '\[SvoTests\]'
    if ($test.ExitCode -ne 0)
    {
        Get-Content -LiteralPath $stderr
        throw "SVO GPU regression failed ($($test.ExitCode)); inspect $stdout and $stderr"
    }
    if ($Validation -and -not (Select-String -LiteralPath $stdout -Pattern '\[SvoTests\] validation=1'))
    {
        throw "Requested Vulkan validation layer was not enabled; inspect $stdout"
    }
    if ((Select-String -LiteralPath $stdout -Pattern '\[Vulkan\].*\[Error\]') -or (Select-String -LiteralPath $stderr -Pattern 'Validation Error|SYNC-HAZARD'))
    {
        throw "Vulkan validation errors; inspect $stdout and $stderr"
    }
    [ordered]@{ configuration = $Configuration; validation = [bool]$Validation; gpu = 'passed'; shaders = 'passed'; stdout = $stdout; stderr = $stderr } | ConvertTo-Json | Set-Content -LiteralPath $summary -Encoding UTF8
}
finally
{
    $env:KAPP_VULKAN_VALIDATION = $previousValidation
    foreach ($path in $savedSettings.Keys) { [System.IO.File]::WriteAllBytes($path, $savedSettings[$path]) }
    foreach ($path in $missingSettings) { if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path } }
    foreach ($path in @($fixture, "$fixture.lod", "$fixture.plain.obj"))
    {
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
    }
}
