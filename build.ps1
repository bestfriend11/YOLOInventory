# Build MyProject4Editor and ShaderCompileWorker using UnrealBuildTool on Windows
# Adjust paths if your engine or project are located elsewhere.

param(
  [string]$UBT,
  [string]$Proj,
  [switch]$KillEditor
)

$ErrorActionPreference = 'Stop'

function Resolve-UProjectPath {
  param([string]$StartDir)
  $dir = Get-Item -LiteralPath $StartDir
  while ($null -ne $dir) {
    $uproject = Get-ChildItem -LiteralPath $dir.FullName -Filter '*.uproject' -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($uproject) { return $uproject.FullName }
    $dir = $dir.Parent
  }
  return $null
}

function Find-UnrealBuildTool {
  # If provided explicitly, use it
  param([string]$Hint)
  if ($Hint -and (Test-Path $Hint)) { return (Resolve-Path $Hint).Path }

  # Common installed build locations
  $candidates = @(
    "D:\Repos\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll",
    "$env:ProgramFiles\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll",
    "$env:ProgramFiles\Epic Games\UE_5.6\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll",
    "$env:ProgramFiles\Epic Games\UE_5.5\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
  )

  foreach ($c in $candidates) { if (Test-Path $c) { return (Resolve-Path $c).Path } }

  # Source build heuristic: UE root adjacent to project (../../..)
  try {
    $root = (Get-Item -LiteralPath $PSScriptRoot).Parent.Parent.FullName
    $srcCandidate = Join-Path $root 'Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll'
    if (Test-Path $srcCandidate) { return (Resolve-Path $srcCandidate).Path }
  } catch {}

  return $null
}

# Resolve defaults when not provided
if (-not $Proj -or -not (Test-Path $Proj)) {
  $Proj = Resolve-UProjectPath -StartDir $PSScriptRoot
}

$UBT = Find-UnrealBuildTool -Hint $UBT

if ([string]::IsNullOrWhiteSpace($UBT) -or -not (Test-Path $UBT)) {
  throw "UnrealBuildTool not found. Pass -UBT 'Full\\Path\\UnrealBuildTool.dll' or install UE 5.7+ (Installed Build)."
}
if ([string]::IsNullOrWhiteSpace($Proj) -or -not (Test-Path $Proj)) {
  throw "Project not found. Pass -Proj 'Full\\Path\\MyProject4.uproject'."
}

if ($KillEditor) {
  Write-Host "KillEditor: attempting to terminate UnrealEditor processes..." -ForegroundColor Yellow
  $processNames = @('UnrealEditor', 'UnrealEditor-Cmd', 'LiveCodingConsole', 'ShaderCompileWorker')
  foreach ($name in $processNames) {
    Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -like "$name*" } | ForEach-Object {
      try {
        Write-Host (" - Killing {0} (PID {1})" -f $_.ProcessName, $_.Id)
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
      } catch {}
    }
  }
  Start-Sleep -Seconds 1
}

# Build commands
$targets = @(
  'MyProject4Editor Win64 Development',
  'ShaderCompileWorker Win64 Development -Quiet'
)

$quotedProj = '"' + ($Proj -replace '"','\"') + '"'
$parts = @('dotnet', '"' + $UBT + '"', ('-Project={0}' -f $quotedProj))
foreach ($t in $targets) {
  $parts += ('-Target="{0}"' -f $t)
}
$parts += @('-WaitMutex','-FromMsBuild','-architecture=x64')
$cmd = ($parts -join ' ')

Write-Host "Running:" $cmd

# Execute the command in a child PowerShell to avoid cmdline quoting issues
$proc = Start-Process -FilePath "powershell" -ArgumentList @('-NoProfile','-Command', $cmd) -NoNewWindow -PassThru -Wait
if ($proc.ExitCode -ne 0) {
  Write-Error "Build failed with exit code $($proc.ExitCode)"
}
exit $proc.ExitCode
