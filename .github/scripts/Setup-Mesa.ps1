param(
    [Parameter(Mandatory = $true)]
    [string]$TestPreset
)

$ErrorActionPreference = 'Stop'
$version = '26.2.0'
$expectedHash = 'dcb2719ef346dab5b609fcb193a5f13cfc4b0502e3f4de1ad43d349477402f47'
$presets = Get-Content -LiteralPath "$PSScriptRoot/../../CMakePresets.json" -Raw | ConvertFrom-Json
$preset = @($presets.testPresets | Where-Object { $_.name -eq $TestPreset })
if ($preset.Count -ne 1 -or -not $preset[0].configuration) {
    throw "Expected one Windows test preset with a configuration: $TestPreset"
}
$outputDirectory = Join-Path "$PSScriptRoot/../../build" "$($preset[0].configurePreset)/bin/$($preset[0].configuration)"
$outputDirectory = (Resolve-Path -LiteralPath $outputDirectory).Path
$mesaDirectory = Join-Path $env:RUNNER_TEMP "mesa-$version"
$archive = Join-Path $env:RUNNER_TEMP "mesa3d-$version-release-msvc.7z"

Invoke-WebRequest -Uri "https://github.com/pal1000/mesa-dist-win/releases/download/$version/mesa3d-$version-release-msvc.7z" -OutFile $archive
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash -ne $expectedHash) {
    throw 'Mesa archive SHA-256 mismatch'
}
New-Item -ItemType Directory -Path $mesaDirectory -Force | Out-Null
Push-Location -LiteralPath $mesaDirectory
try {
    cmake -E tar xf $archive
    if ($LASTEXITCODE -ne 0) {
        throw 'Mesa archive extraction failed'
    }
} finally {
    Pop-Location
}

# Deploy only beside these binaries; never replace Windows system DLLs.
foreach ($name in @('opengl32.dll', 'libgallium_wgl.dll', 'dxil.dll')) {
    Copy-Item -LiteralPath (Join-Path $mesaDirectory "x64/$name") -Destination $outputDirectory -Force
}
Write-Host "Mesa $version (SHA-256 $expectedHash) deployed to $outputDirectory"
