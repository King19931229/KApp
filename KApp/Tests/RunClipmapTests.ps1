param([switch]$SkipBuild, [switch]$Validation, [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release', [string]$Scene = '')

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $SkipBuild)
{
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vsRoot = & $vswhere -version '[16.0,17.0)' -latest -property installationPath
    $msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
    $toolVersion = (Get-Content -LiteralPath (Join-Path $vsRoot 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
    foreach ($project in @('KBase\KBase.vcxproj', 'KRender\KRender.vcxproj', 'KEngine\KEngine.vcxproj', 'KApp\Tests\KClipmapTests.vcxproj'))
    {
        & $msbuild (Join-Path $repoRoot $project) /t:Build "/p:Configuration=$Configuration" /p:Platform=x64 /p:PlatformToolset=v142 "/p:VCToolsVersion=$toolVersion" /p:PreferredToolArchitecture=x64 /p:BuildProjectReferences=false /m:4 /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { throw "Build failed: $project" }
    }
}
$binaryDir = Join-Path $repoRoot ("Product\Binaries\x64\" + $Configuration)
$logDir = Join-Path $PSScriptRoot ("bin\Clipmap\" + $Configuration)
if ($Scene) { $Scene = (Resolve-Path -LiteralPath $Scene).Path; $logDir = Join-Path $logDir ('production-' + [System.IO.Path]::GetFileNameWithoutExtension($Scene)) }
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$stdout = Join-Path $logDir 'stdout.log'
$stderr = Join-Path $logDir 'stderr.log'
$fixture = Join-Path $logDir 'clipmap-fixture.obj'
$previousValidation = $env:KAPP_VULKAN_VALIDATION
$savedSettings = @{}
foreach ($name in @('render_gui.json', 'imgui.ini'))
{
    $path = Join-Path $binaryDir $name
    if (Test-Path -LiteralPath $path) { $savedSettings[$path] = [System.IO.File]::ReadAllBytes($path) }
}
try
{
    $env:KAPP_VULKAN_VALIDATION = if ($Validation) { '1' } else { '0' }
    $testArguments = '"' + $fixture + '"'
    if ($Scene) { $testArguments += ' "' + $Scene + '"' }
    $test = Start-Process -FilePath (Join-Path $binaryDir 'KClipmapTests.exe') -ArgumentList $testArguments -WorkingDirectory $binaryDir -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    while (-not $test.WaitForExit(1000))
    {
        if ((Get-Date) - $test.StartTime -gt [TimeSpan]::FromMinutes(8))
        {
            $test.Kill()
            throw "Clipmap GI regression timed out; inspect $stdout and $stderr"
        }
    }
    Get-Content -LiteralPath $stdout | Select-String -Pattern '\[ClipmapTests\]'
    if ($test.ExitCode -ne 0)
    {
        Get-Content -LiteralPath $stderr
        throw "Clipmap GI regression failed ($($test.ExitCode)); inspect $stdout and $stderr"
    }
    if (Select-String -LiteralPath $stdout -Pattern '\[Vulkan\].*\[Error\]')
    {
        throw "Vulkan validation errors; inspect $stdout"
    }
}
finally
{
    $env:KAPP_VULKAN_VALIDATION = $previousValidation
    foreach ($path in $savedSettings.Keys) { [System.IO.File]::WriteAllBytes($path, $savedSettings[$path]) }
    foreach ($path in @($fixture, "$fixture.lod", "$fixture.plain.obj"))
    {
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
    }
}
