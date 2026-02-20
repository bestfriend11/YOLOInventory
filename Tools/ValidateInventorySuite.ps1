param(
    [switch]$FailOnWarning
)

$ErrorActionPreference = "Stop"

function Get-BuildModuleDependencies {
    param([string]$BuildFilePath)

    $content = Get-Content $BuildFilePath -Raw
    $moduleName = [System.IO.Path]::GetFileNameWithoutExtension($BuildFilePath)
    if ($moduleName.EndsWith(".Build")) {
        $moduleName = $moduleName.Substring(0, $moduleName.Length - ".Build".Length)
    }
    $quoted = [regex]::Matches($content, '"([^"]+)"') | ForEach-Object { $_.Groups[1].Value }
    $deps = $quoted | Where-Object { $_ -match '^YOLOInventory' } | Sort-Object -Unique
    return [PSCustomObject]@{
        Module = $moduleName
        BuildFile = $BuildFilePath
        Dependencies = $deps
    }
}

function Test-ForbiddenDependencies {
    param(
        [string]$Module,
        [string[]]$Dependencies,
        [string[]]$ForbiddenPatterns
    )

    $hits = @()
    foreach ($dep in $Dependencies) {
        foreach ($pattern in $ForbiddenPatterns) {
            if ($dep -like $pattern) {
                $hits += $dep
                break
            }
        }
    }
    return $hits | Sort-Object -Unique
}

$projectRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$suiteRoot = Join-Path $projectRoot "Plugins\YOLO"

$buildFiles = @(
    Get-ChildItem $suiteRoot -Recurse -Filter "*.Build.cs" | ForEach-Object { $_.FullName }
)

$legacyBuild = Join-Path $projectRoot "Plugins\YOLOInventory\Source\YOLOInventory\YOLOInventory.Build.cs"
if (Test-Path $legacyBuild) {
    $buildFiles += $legacyBuild
}

$modules = @{}
foreach ($buildFile in $buildFiles) {
    $entry = Get-BuildModuleDependencies -BuildFilePath $buildFile
    $modules[$entry.Module] = $entry
}

$errors = New-Object System.Collections.Generic.List[string]
$warnings = New-Object System.Collections.Generic.List[string]

$rules = @(
    @{ Module = "YOLOInventoryCore"; Forbidden = @("YOLOInventorySchema","YOLOInventoryContainers","YOLOInventoryGrid","YOLOInventoryEquipment","YOLOInventoryWorld","YOLOInventoryLoot","YOLOInventoryTrade","YOLOInventoryUI","YOLOInventoryTemplateDS1*","YOLOInventoryEditor*","YOLOInventoryLegacyBridge","YOLOInventory") },
    @{ Module = "YOLOInventorySchema"; Forbidden = @("YOLOInventoryContainers","YOLOInventoryGrid","YOLOInventoryEquipment","YOLOInventoryWorld","YOLOInventoryLoot","YOLOInventoryTrade","YOLOInventoryUI","YOLOInventoryTemplateDS1*","YOLOInventoryEditor*","YOLOInventoryLegacyBridge","YOLOInventory") },
    @{ Module = "YOLOInventoryGrid"; Forbidden = @("YOLOInventorySchema","YOLOInventoryContainers","YOLOInventoryEquipment","YOLOInventoryWorld","YOLOInventoryLoot","YOLOInventoryTrade","YOLOInventoryUI","YOLOInventoryTemplateDS1*","YOLOInventoryEditor*","YOLOInventoryLegacyBridge","YOLOInventory") },
    @{ Module = "YOLOInventoryContainers"; Forbidden = @("YOLOInventoryGrid","YOLOInventoryUI","YOLOInventoryTemplateDS1*","YOLOInventoryEditor*","YOLOInventoryLegacyBridge","YOLOInventory") },
    @{ Module = "YOLOInventoryEquipment"; Forbidden = @("YOLOInventoryUI","YOLOInventoryTemplateDS1*","YOLOInventoryEditor*","YOLOInventoryLegacyBridge","YOLOInventory") },
    @{ Module = "YOLOInventoryWorld"; Forbidden = @("YOLOInventoryUI","YOLOInventoryTemplateDS1*","YOLOInventoryEditor*","YOLOInventoryLegacyBridge","YOLOInventory") },
    @{ Module = "YOLOInventoryLoot"; Forbidden = @("YOLOInventoryUI","YOLOInventoryTemplateDS1*","YOLOInventoryEditor*","YOLOInventoryLegacyBridge","YOLOInventory") },
    @{ Module = "YOLOInventoryTrade"; Forbidden = @("YOLOInventoryTemplateDS1*","YOLOInventoryEditor*","YOLOInventoryLegacyBridge","YOLOInventory") }
)

foreach ($rule in $rules) {
    if (-not $modules.ContainsKey($rule.Module)) {
        $warnings.Add("Missing module for validation rule: $($rule.Module)")
        continue
    }

    $entry = $modules[$rule.Module]
    $hits = Test-ForbiddenDependencies -Module $rule.Module -Dependencies $entry.Dependencies -ForbiddenPatterns $rule.Forbidden
    if ($hits.Count -gt 0) {
        $errors.Add("$($rule.Module) depends on forbidden modules: $($hits -join ', ')")
    }
}

# Legacy bridge is allowed to depend on runtime modules, but should not leak editor dependencies.
if ($modules.ContainsKey("YOLOInventoryLegacyBridge")) {
    $entry = $modules["YOLOInventoryLegacyBridge"]
    $hits = Test-ForbiddenDependencies -Module "YOLOInventoryLegacyBridge" -Dependencies $entry.Dependencies -ForbiddenPatterns @("YOLOInventoryEditor*","YOLOInventoryTemplateDS1*")
    if ($hits.Count -gt 0) {
        $errors.Add("YOLOInventoryLegacyBridge depends on editor/template modules: $($hits -join ', ')")
    }
}

# Legacy runtime shell should stay minimal.
if ($modules.ContainsKey("YOLOInventory")) {
    $entry = $modules["YOLOInventory"]
    $allowed = @("YOLOInventoryCore")
    $extra = $entry.Dependencies | Where-Object { $allowed -notcontains $_ }
    if ($extra.Count -gt 0) {
        $errors.Add("Legacy YOLOInventory module has unexpected suite deps: $($extra -join ', ')")
    }
}

# Non-template modules should avoid DS1-specific naming in source comments/docs.
$sourceFiles = Get-ChildItem $suiteRoot -Recurse -Include *.h,*.cpp,*.md | Where-Object {
    $_.FullName -notmatch "\\YOLOInventoryTemplateDS1\\" -and
    $_.FullName -notmatch "\\Intermediate\\" -and
    $_.FullName -notmatch "\\Binaries\\"
}

$opinionatedHits = @()
foreach ($file in $sourceFiles) {
    $matches = Select-String -Path $file.FullName -Pattern 'DungeonSiege|\bDS1\b' -SimpleMatch:$false
    foreach ($m in $matches) {
        $rel = $m.Path.Replace($projectRoot.Path + "\", "")
        $opinionatedHits += "${rel}:$($m.LineNumber)"
    }
}

if ($opinionatedHits.Count -gt 0) {
    foreach ($hit in ($opinionatedHits | Sort-Object -Unique)) {
        $warnings.Add("Template-specific wording outside template plugin: $hit")
    }
}

Write-Host "=== YOLO Inventory Suite Validation ==="
Write-Host "Modules scanned: $($modules.Keys.Count)"

if ($errors.Count -gt 0) {
    Write-Host ""
    Write-Host "Errors:" -ForegroundColor Red
    foreach ($e in $errors) {
        Write-Host "  - $e" -ForegroundColor Red
    }
}

if ($warnings.Count -gt 0) {
    Write-Host ""
    Write-Host "Warnings:" -ForegroundColor Yellow
    foreach ($w in $warnings) {
        Write-Host "  - $w" -ForegroundColor Yellow
    }
}

if ($errors.Count -eq 0 -and ($warnings.Count -eq 0 -or -not $FailOnWarning)) {
    Write-Host ""
    Write-Host "Validation passed." -ForegroundColor Green
    exit 0
}

if ($errors.Count -gt 0) {
    exit 1
}

if ($FailOnWarning -and $warnings.Count -gt 0) {
    exit 2
}

exit 0
