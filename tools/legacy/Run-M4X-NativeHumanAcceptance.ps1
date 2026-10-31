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
$mission = '{A4F2600000000000}Missions/TB_CC_M4X_NativePrototype.conf'
$repoRoot = Split-Path -Parent $PSScriptRoot
$workspaceAddonsDirectory = Split-Path -Parent $repoRoot
$runRoot = Join-Path $EvidenceDirectory ('M4X_HumanAcceptance_' + (Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'))
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

function Wait-ForServerReady {
    param([Diagnostics.Process]$Process)
    $deadline = (Get-Date).AddSeconds($ServerReadyTimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if ($Process.HasExited) { throw "ServerDiag exited before readiness. Server profile: $serverProfile" }
        $content = Read-ConsoleLog -Profile $serverProfile
        if ($content -match 'Entered online game state\.' -and
            $content -match 'Starting RPL server, listening on address .*:2001' -and
            $content -match '\[TB_CC_M4_1\] READY map=ARLAND_NATIVE_PROTO_01.*positions=3 edges=2 mobs=1' -and
            $content -match '\[TB_CC_M4_4\] deployment-policy selected=PROTO::MOB enabledUSMobSpawnPoints=1 allForwardDisabled=1' -and
            $content -match '\[TB_CC_M4X\] state ready=1 revision=1 USLegal=PROTO::OBJ_01.*depth=0') { return }
        Start-Sleep -Milliseconds 500
    }
    throw "ServerDiag did not reach the positive M4.X native ready state within $ServerReadyTimeoutSeconds seconds. Server profile: $serverProfile"
}

foreach ($requiredFile in @(
    $ServerExecutable,
    $ClientExecutable,
    (Join-Path $repoRoot 'Missions\TB_CC_M4X_NativePrototype.conf'),
    (Join-Path $repoRoot 'Worlds\TB_CC_M4X_NativePrototype.ent'),
    (Join-Path $repoRoot 'Worlds\TB_CC_M4X_NativePrototype_Layers\Native.layer')
)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) { throw "Required file not found: $requiredFile" }
}
foreach ($requiredDirectory in @($ServerAddonsDirectory, $ClientAddonsDirectory, $workspaceAddonsDirectory)) {
    if (-not (Test-Path -LiteralPath $requiredDirectory -PathType Container)) { throw "Required addon directory not found: $requiredDirectory" }
}

$missionText = Get-Content -LiteralPath (Join-Path $repoRoot 'Missions\TB_CC_M4X_NativePrototype.conf') -Raw
$layerText = Get-Content -LiteralPath (Join-Path $repoRoot 'Worlds\TB_CC_M4X_NativePrototype_Layers\Native.layer') -Raw
if ($missionText -notmatch [regex]::Escape('{A4F2300000000000}Worlds/TB_CC_M4X_NativePrototype.ent')) {
    throw 'The native mission no longer points to the expected prototype world.'
}
foreach ($requiredSetting in @(
    'm_iAuthorityProofMode 0',
    'm_iBLUFORVictoryHoldSeconds 300',
    'm_iOPFORVictoryHoldSeconds 300',
    'm_bEnableM3Coordinator 1',
    'm_bEnableM3Garrison 1',
    'TB_CC_NATIVE_RADIO_MANAGER',
    'TB_CC_NATIVE_LOADOUT_MANAGER'
)) {
    if (-not $layerText.Contains($requiredSetting)) { throw "Native world preflight missing: $requiredSetting" }
}
if ($layerText -match 'TB_CC_M4X_PhysicalProofComponent|TB_CC_M4X_DependencyAuditComponent|TB_CC_M4X_PressureProof') {
    throw 'A diagnostic actor reference was found in the normal native layer.'
}

$existingDiagnostics = @(Get-Process -Name 'ArmaReforgerServerDiag', 'ArmaReforgerSteamDiag' -ErrorAction SilentlyContinue)
if ($existingDiagnostics.Count -gt 0) {
    $details = ($existingDiagnostics | ForEach-Object { "$($_.ProcessName) PID=$($_.Id)" }) -join '; '
    throw "Close existing diagnostic Reforger processes before this isolated run: $details"
}
if (@(Get-Process -Name 'steam' -ErrorAction SilentlyContinue).Count -eq 0) {
    throw 'Steam is not running. Start it and sign in before launching the interactive client.'
}

Write-Host 'M4.X NATIVE HUMAN ACCEPTANCE — ONE CLIENT'
Write-Host "Mission: $mission"
Write-Host "Addon: $addonId"
Write-Host "Working directory: $repoRoot"
Write-Host "Run root: $runRoot"
Write-Host "Server profile/logs: $serverProfile"
Write-Host "Client profile/logs: $clientProfile"
Write-Host "Server launch: `"$ServerExecutable`" $(ConvertTo-ArgumentString $serverArguments)"
Write-Host "Client launch: `"$ClientExecutable`" $(ConvertTo-ArgumentString $clientArguments)"
Write-Host "Server age target before client: $ServerAgeBeforeClientSeconds seconds"
if ($DryRun) {
    Write-Host 'DRY RUN: no directories or processes created.'
    return
}

if (Test-Path -LiteralPath $runRoot) { throw "Run directory already exists; refusing to reuse profiles: $runRoot" }
New-Item -ItemType Directory -Path $serverProfile, $clientProfile -Force | Out-Null
$serverStart = Get-Date
$server = Start-Process -FilePath $ServerExecutable -ArgumentList (ConvertTo-ArgumentString $serverArguments) -WorkingDirectory $repoRoot -WindowStyle Hidden -PassThru
Write-Host "ServerDiag started PID=$($server.Id); waiting for online native scenario readiness..."
Wait-ForServerReady -Process $server

$clientTargetTime = $serverStart.AddSeconds($ServerAgeBeforeClientSeconds)
while ((Get-Date) -lt $clientTargetTime) {
    if ($server.HasExited) { throw "ServerDiag exited during the settle interval. Server profile: $serverProfile" }
    $sleepMilliseconds = [math]::Min(1000, [math]::Max(1, [int](($clientTargetTime - (Get-Date)).TotalMilliseconds)))
    Start-Sleep -Milliseconds $sleepMilliseconds
}
if ($server.HasExited) { throw "ServerDiag exited before client launch. Server profile: $serverProfile" }
$clientLaunch = Get-Date
$client = Start-Process -FilePath $ClientExecutable -ArgumentList (ConvertTo-ArgumentString $clientArguments) -WorkingDirectory $repoRoot -PassThru
Write-Host "One visible SteamDiag client launched PID=$($client.Id) at server age $([math]::Round(($clientLaunch - $serverStart).TotalSeconds, 3)) seconds."

if ($AuthenticationObservationSeconds -gt 0) {
    $deadline = (Get-Date).AddSeconds($AuthenticationObservationSeconds)
    while ((Get-Date) -lt $deadline) {
        $serverText = Read-ConsoleLog -Profile $serverProfile
        $clientText = Read-ConsoleLog -Profile $clientProfile
        if ($serverText -match 'Players connected:\s*1\s*/\s*\d+') {
            Write-Host 'ONE CLIENT ADMITTED: server reports one connected player.'
            break
        }
        if ($serverText -match '(?i)(group\s*[=:]\s*4.*reason\s*[=:]\s*3|authorization.*(failed|rejected))' -or
            $clientText -match '(?i)(SteamAPI_Init failed|authorization.*(failed|rejected))') {
            Write-Warning 'Authentication rejection observed. No retry will be started.'
            break
        }
        if ($client.HasExited) {
            Write-Warning 'The interactive client exited before admission. Inspect both logs; no retry will be started.'
            break
        }
        Start-Sleep -Milliseconds 500
    }
}

Write-Host "Server PID: $($server.Id)"
Write-Host "Client PID: $($client.Id)"
$serverLog = Get-ConsoleLog -Profile $serverProfile
$clientLog = Get-ConsoleLog -Profile $clientProfile
if ($serverLog) { Write-Host "Server log: $($serverLog.FullName)" }
else { Write-Warning "Server console.log not found yet under: $serverProfile" }
if ($clientLog) { Write-Host "Client log: $($clientLog.FullName)" }
else { Write-Warning "Client console.log not found yet under: $clientProfile" }
Write-Host 'The server and client remain running for manual play and later log analysis.'
