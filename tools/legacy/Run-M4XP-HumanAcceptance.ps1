[CmdletBinding()]
param(
    [switch]$DryRun,
    [string]$ServerExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe',
    [string]$ClientExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger\ArmaReforgerSteamDiag.exe',
    [string]$ServerAddonsDirectory = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\addons',
    [string]$ClientAddonsDirectory = 'D:\SteamLibrary\steamapps\common\Arma Reforger\addons',
    [string]$EvidenceDirectory = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'BCM-Conflict-Core-TestEvidence'),
    [ValidateRange(30, 600)][int]$ServerReadyTimeoutSeconds = 180,
    [ValidateRange(30, 600)][double]$ServerAgeBeforeClientSeconds = 257.705,
    [ValidateRange(0, 120)][int]$AuthenticationObservationSeconds = 30
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$addonId = '6A6A5973FA53A5B1'
$mission = '{A4F3500000000000}Missions/Production/TB_CC_Arland.conf'
$repoRoot = Split-Path -Parent $PSScriptRoot
$workspaceAddonsDirectory = Split-Path -Parent $repoRoot
$runRoot = Join-Path $EvidenceDirectory ('M4XP_HumanAcceptance_' + (Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'))
$serverProfile = Join-Path $runRoot 'Server'
$clientProfile = Join-Path $runRoot 'Client'
$serverAddons = "$ServerAddonsDirectory,$workspaceAddonsDirectory"
$clientAddons = "$ClientAddonsDirectory,$workspaceAddonsDirectory"
$serverArguments = @('-server', $mission, '-addonsDir', $serverAddons, '-addons', $addonId, '-profile', $serverProfile, '-maxFPS', '60', '-logLevel', 'debug')
$clientArguments = @('-client', '127.0.0.1', '-addonsDir', $clientAddons, '-addons', $addonId, '-profile', $clientProfile, '-logLevel', 'debug')

function ConvertTo-ArgumentString {
    param([string[]]$Arguments)
    $rendered = foreach ($argument in $Arguments) {
        if ($argument -match '[\s,]') { '"' + $argument.Replace('"', '\"') + '"' } else { $argument }
    }
    return $rendered -join ' '
}
function Get-ConsoleLog {
    param([string]$Profile)
    $logDirectory = Join-Path $Profile 'logs'
    if (-not (Test-Path -LiteralPath $logDirectory -PathType Container)) { return $null }
    Get-ChildItem -LiteralPath $logDirectory -Recurse -File -Filter 'console.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
}
function Read-ConsoleLog {
    param([string]$Profile)
    $log = Get-ConsoleLog -Profile $Profile
    if (-not $log) { return '' }
    try { return Get-Content -LiteralPath $log.FullName -Raw -ErrorAction Stop }
    catch { return '' }
}

foreach ($path in @(
    $ServerExecutable, $ClientExecutable,
    (Join-Path $repoRoot 'Missions\Production\TB_CC_Arland.conf'),
    (Join-Path $repoRoot 'Worlds\Production\Arland\TB_CC_Arland.ent'),
    (Join-Path $repoRoot 'Worlds\Production\Arland\TB_CC_Arland_Layers\A_Managers.layer'),
    (Join-Path $repoRoot 'Worlds\Production\Arland\TB_CC_Arland_Layers\B_MOB.layer'),
    (Join-Path $repoRoot 'Worlds\Production\Arland\TB_CC_Arland_Layers\C_CapturePoints.layer')
)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file missing: $path" }
}
foreach ($path in @($ServerAddonsDirectory, $ClientAddonsDirectory, $workspaceAddonsDirectory)) {
    if (-not (Test-Path -LiteralPath $path -PathType Container)) { throw "Addon directory missing: $path" }
}
$missionText = Get-Content -LiteralPath (Join-Path $repoRoot 'Missions\Production\TB_CC_Arland.conf') -Raw
$layerText = (Get-ChildItem -LiteralPath (Join-Path $repoRoot 'Worlds\Production\Arland\TB_CC_Arland_Layers') -Filter '*.layer' -File | ForEach-Object { Get-Content -LiteralPath $_.FullName -Raw }) -join "`n"
if ($missionText -notmatch [regex]::Escape('{A4F3300000000000}Worlds/Production/Arland/TB_CC_Arland.ent')) { throw 'Full-native mission/world mismatch.' }
foreach ($setting in @('m_iAuthorityProofMode 0', 'm_bEnableM3Coordinator 1', 'm_bEnableM3Garrison 1')) {
    if (-not $layerText.Contains($setting)) { throw "Production setting missing: $setting" }
}
if ($layerText -match 'TB_CC_M4XP_IntegratedProofComponent|TB_CC_M4X_PhysicalProofComponent') { throw 'Proof actor found in production world.' }
. (Join-Path $PSScriptRoot 'Assert-NativePressureConfiguration.ps1')
$pressureDuration = Assert-NativePressureConfiguration -RepositoryRoot $repoRoot
Write-Host "Native pressure hold: $pressureDuration seconds"

Write-Host 'M4.XP FULL-NATIVE HUMAN ACCEPTANCE — ONE INTERACTIVE CLIENT'
Write-Host "Mission: $mission"
Write-Host "Addon: $addonId"
Write-Host "Working directory: $repoRoot"
Write-Host "Server profile/logs: $serverProfile"
Write-Host "Client profile/logs: $clientProfile"
Write-Host "Server launch: `"$ServerExecutable`" $(ConvertTo-ArgumentString $serverArguments)"
Write-Host "Client launch: `"$ClientExecutable`" $(ConvertTo-ArgumentString $clientArguments)"
Write-Host "Server age before client: $ServerAgeBeforeClientSeconds seconds"
if ($DryRun) { Write-Host 'DRY RUN: no directories or processes created.'; return }

$existing = @(Get-Process -Name 'ArmaReforgerServerDiag', 'ArmaReforgerSteamDiag' -ErrorAction SilentlyContinue)
if ($existing.Count -gt 0) {
    $details = ($existing | ForEach-Object { "$($_.ProcessName) PID=$($_.Id)" }) -join '; '
    throw "Close existing diagnostic Reforger sessions yourself before this isolated run: $details"
}
if (@(Get-Process -Name 'steam' -ErrorAction SilentlyContinue).Count -eq 0) { throw 'Steam must already be running and signed in.' }
if (Test-Path -LiteralPath $runRoot) { throw "Fresh profile path already exists: $runRoot" }
New-Item -ItemType Directory -Path $serverProfile, $clientProfile -Force | Out-Null

$serverStart = Get-Date
$server = Start-Process -FilePath $ServerExecutable -ArgumentList (ConvertTo-ArgumentString $serverArguments) -WorkingDirectory $repoRoot -WindowStyle Hidden -PassThru
Write-Host "ServerDiag PID=$($server.Id); waiting for online full-native readiness..."
$readyDeadline = (Get-Date).AddSeconds($ServerReadyTimeoutSeconds)
while ((Get-Date) -lt $readyDeadline) {
    if ($server.HasExited) { throw "ServerDiag exited before readiness. Logs: $serverProfile" }
    $content = Read-ConsoleLog -Profile $serverProfile
    if ($content -match 'Entered online game state\.' -and
        $content -match '\[TB_CC_M4_1\] READY map=ARLAND_NATIVE_FULL_01.*positions=12 edges=21' -and
        $content -match '\[TB_CC_M4_4\] deployment-policy selected=ARLAND::HQ_.*enabledUSMobSpawnPoints=1 allForwardDisabled=1' -and
        $content -match '\[TB_CC_M4XP\] state ready=1 revision=1 outcome=1 depth=0 ordinaryUS=0 ordinaryUSSR=11 required=11 foothold=0 pressure=0') { break }
    Start-Sleep -Milliseconds 500
}
if ((Get-Date) -ge $readyDeadline) { throw "Positive full-native readiness not observed. Logs: $serverProfile" }

$clientTargetTime = $serverStart.AddSeconds($ServerAgeBeforeClientSeconds)
while ((Get-Date) -lt $clientTargetTime) {
    if ($server.HasExited) { throw "ServerDiag exited during settle interval. Logs: $serverProfile" }
    Start-Sleep -Milliseconds 1000
}
if ($server.HasExited) { throw "ServerDiag exited before client launch. Logs: $serverProfile" }
$clientStart = Get-Date
$client = Start-Process -FilePath $ClientExecutable -ArgumentList (ConvertTo-ArgumentString $clientArguments) -WorkingDirectory $repoRoot -PassThru
Write-Host "One visible SteamDiag client PID=$($client.Id) launched at server age $([math]::Round(($clientStart - $serverStart).TotalSeconds, 3)) seconds."

$authDeadline = (Get-Date).AddSeconds($AuthenticationObservationSeconds)
while ((Get-Date) -lt $authDeadline) {
    $serverText = Read-ConsoleLog -Profile $serverProfile
    $clientText = Read-ConsoleLog -Profile $clientProfile
    if ($serverText -match 'Players connected:\s*1\s*/\s*\d+') { Write-Host 'ONE CLIENT ADMITTED: server reports one connected player.'; break }
    if ($serverText -match '(?i)(authorization.*(failed|rejected)|reason\s*[=:]\s*3)' -or $clientText -match '(?i)(SteamAPI_Init failed|authorization.*(failed|rejected))') { Write-Warning 'Authentication rejected; no retry will be started.'; break }
    if ($client.HasExited) { Write-Warning 'Client exited before admission; no retry will be started.'; break }
    Start-Sleep -Milliseconds 500
}

Write-Host "Server PID: $($server.Id)"
Write-Host "Client PID: $($client.Id)"
$serverLog = Get-ConsoleLog -Profile $serverProfile
$clientLog = Get-ConsoleLog -Profile $clientProfile
if ($serverLog) { Write-Host "Server log: $($serverLog.FullName)" }
if ($clientLog) { Write-Host "Client log: $($clientLog.FullName)" }
Write-Host 'Both processes remain running for the Creative Director human session. No second client was launched.'
