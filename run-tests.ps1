# Run YOLOInventory automation tests headlessly (no editor UI).
# Usage:
#   ./run-tests.ps1 -Tests "YOLOInventory.*" -ReportDir "D:\aa\TestReports" -KillEditor
# Defaults: Tests = "YOLOInventory.*", ReportDir = "$PSScriptRoot\TestReports"

param(
  [string]$UECmd,
  [string]$Proj,
  [string]$Tests = "YOLOInventory.*",
  [string]$ReportDir,
  [switch]$KillEditor,
  [switch]$UseExecCmd
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

function Find-UnrealEditorCmd {
  param([string]$Hint)
  if ($Hint -and (Test-Path $Hint)) { return (Resolve-Path $Hint).Path }

  $candidates = @(
    "D:\Repos\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
    "$env:ProgramFiles\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
    "$env:ProgramFiles\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
    "$env:ProgramFiles\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
  )
  foreach ($c in $candidates) { if (Test-Path $c) { return (Resolve-Path $c).Path } }
  return $null
}

if (-not $Proj -or -not (Test-Path $Proj)) {
  $Proj = Resolve-UProjectPath -StartDir $PSScriptRoot
}
if (-not $ReportDir -or [string]::IsNullOrWhiteSpace($ReportDir)) {
  $ReportDir = Join-Path $PSScriptRoot "TestReports"
}
# Prefer exec-cmd flow by default (AutomationCommandlet missing in some builds)
if (-not $PSBoundParameters.ContainsKey('UseExecCmd')) { $UseExecCmd = $true }

$UECmd = Find-UnrealEditorCmd -Hint $UECmd

if ([string]::IsNullOrWhiteSpace($UECmd) -or -not (Test-Path $UECmd)) {
  throw "UnrealEditor-Cmd.exe not found. Pass -UECmd 'Full\Path\UnrealEditor-Cmd.exe' or adjust the search paths in this script."
}
if ([string]::IsNullOrWhiteSpace($Proj) -or -not (Test-Path $Proj)) {
  throw "Project not found. Pass -Proj 'Full\Path\MyProject4.uproject'."
}

if ($KillEditor) {
  Write-Host "KillEditor: terminating UnrealEditor processes..." -ForegroundColor Yellow
  Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -like "UnrealEditor*" } | ForEach-Object {
    try { Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue; Write-Host " - Killed $($_.ProcessName) ($($_.Id))" } catch {}
  }
}

if (-not (Test-Path $ReportDir)) {
  New-Item -ItemType Directory -Path $ReportDir | Out-Null
}

if ($UseExecCmd) {
  $args = @(
    "`"$Proj`"",
    "-unattended",
    "-nopause",
    "-nullrhi",
    "-log",
    "-ReportExportPath=`"$ReportDir`"",
    "-ExecCmds=`"Automation RunTests $Tests; Quit`""
  )
} else {
  $args = @(
    "`"$Proj`"",
    "-run=Automation",
    "-Test=$Tests",
    "-unattended",
    "-nopause",
    "-nullrhi",
    "-log",
    "-ReportExportPath=`"$ReportDir`"",
    "-ScriptsForAutomation"
  )
}

Write-Host "Running tests:" $UECmd $args -ForegroundColor Cyan
$proc = Start-Process -FilePath $UECmd -ArgumentList $args -NoNewWindow -PassThru -Wait
if ($proc.ExitCode -ne 0) {
  Write-Error "Tests failed with exit code $($proc.ExitCode)"
}
exit $proc.ExitCode
