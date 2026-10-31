[CmdletBinding()]
param(
    [switch]$DryRun,
    [string]$ServerExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe',
    [string]$ConfigPath,
    [string]$EvidenceDirectory = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'BCM-Conflict-Core-TestEvidence'),
    [ValidateRange(30, 600)][int]$ReadyTimeoutSeconds = 240
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$serverDirectory = Split-Path -Parent $ServerExecutable
$configPath = $ConfigPath
if ([string]::IsNullOrWhiteSpace($configPath)) { $configPath = Join-Path $repoRoot 'ServerConfigs\M4XP_IntegratedAuthority.json' }
$missionPath = Join-Path $repoRoot 'Missions\Production\TB_CC_Arland.conf'
$worldPath = Join-Path $repoRoot 'Worlds\Production\Arland\TB_CC_Arland.ent'
$layerPath = Join-Path $repoRoot 'Worlds\Production\Arland\TB_CC_Arland_Layers\A_Managers.layer'
$runRoot = Join-Path $EvidenceDirectory ('M4XP_Phase1_' + (Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'))
$serverProfile = Join-Path $runRoot 'Server'
$addonDirectories = "$serverDirectory\addons,$(Split-Path -Parent $repoRoot)"

foreach ($path in @($ServerExecutable, $configPath, $missionPath, $worldPath, $layerPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file missing: $path" }
}
$config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
if ($config.game.scenarioId -ne '{A4F3500000000000}Missions/Production/TB_CC_Arland.conf') { throw 'Wrong mission in server config.' }
if ($config.game.mods.Count -ne 1 -or $config.game.mods[0].modId -ne '6A6A5973FA53A5B1') { throw 'Wrong addon in server config.' }
if ($config.game.visible) { throw 'Authority server must not be advertised.' }
$mission = Get-Content -LiteralPath $missionPath -Raw
if ($mission -notmatch [regex]::Escape('{A4F3300000000000}Worlds/Production/Arland/TB_CC_Arland.ent')) { throw 'Mission does not point to the native candidate world.' }
$world = Get-Content -LiteralPath $worldPath -Raw
if ($world -match 'CTL_Campaign_Arland' -or $world -notmatch 'Parent "worlds/Arland/Arland.ent"') { throw 'Unexpected world parent.' }
$layer = Get-Content -LiteralPath $layerPath -Raw
if (@([regex]::Matches($layer, '(?m)^SCR_MapEntity\s+\S+\s*\{')).Count -ne 0) { throw 'Native layer must inherit Arland MapEntity, not author a duplicate.' }
foreach ($setting in @('m_iAuthorityProofMode 0', 'm_bEnableM3Coordinator 1', 'm_bEnableM3Garrison 1')) {
    if (-not $layer.Contains($setting)) { throw "Production setting missing: $setting" }
}
if ($layer -match 'TB_CC_M4X_PhysicalProofComponent|TB_CC_M4X_DependencyAuditComponent|TB_CC_M4X_PressureProof') { throw 'Proof actor detected in production layer.' }
. (Join-Path $PSScriptRoot 'Assert-NativePressureConfiguration.ps1')
$pressureDuration = Assert-NativePressureConfiguration -RepositoryRoot $repoRoot
Write-Host "Native pressure hold: $pressureDuration seconds"
foreach ($port in @([int]$config.bindPort, [int]$config.a2s.port)) {
    if (Get-NetUDPEndpoint -LocalPort $port -ErrorAction SilentlyContinue) { throw "UDP port $port is already in use. Do not stop the owner; choose another clean slot deliberately." }
}

$arguments = @('-config', $configPath, '-addonsDir', $addonDirectories, '-profile', $serverProfile, '-maxFPS', '60', '-logLevel', 'debug')
function Quote-Argument([string]$value) {
    if ($value -match '[\s,]') { return '"' + $value.Replace('"', '\"') + '"' }
    return $value
}
$argumentString = (($arguments | ForEach-Object { Quote-Argument $_ }) -join ' ')
Write-Host 'M4.XP PHASE 1 — UNRESTRICTED MANUAL SERVERDIAG'
Write-Host "Executable: $ServerExecutable"
Write-Host "Working directory: $serverDirectory"
Write-Host "Mission: $($config.game.scenarioId)"
Write-Host "Game/query ports: $($config.bindPort)/$($config.a2s.port) UDP"
Write-Host "Profile/logs: $serverProfile"
Write-Host "Command: `"$ServerExecutable`" $argumentString"
if ($DryRun) { Write-Host 'DRY RUN: no process or profile created.'; return }

if (Test-Path -LiteralPath $runRoot) { throw "Run directory already exists: $runRoot" }
New-Item -ItemType Directory -Path $serverProfile -Force | Out-Null
$process = Start-Process -FilePath $ServerExecutable -ArgumentList $argumentString -WorkingDirectory $serverDirectory -WindowStyle Hidden -PassThru
Write-Host "Started isolated ServerDiag PID=$($process.Id). No other process was stopped."
$deadline = (Get-Date).AddSeconds($ReadyTimeoutSeconds)
while ((Get-Date) -lt $deadline) {
    $log = Get-ChildItem -LiteralPath (Join-Path $serverProfile 'logs') -Recurse -File -Filter 'console.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if ($log) {
        $content = Get-Content -LiteralPath $log.FullName -Raw -ErrorAction SilentlyContinue
        if ($content -match 'Entered online game state\.') {
            Write-Host 'ONLINE STATE: PASS. Preserve the server and inspect this log for Phase-1 mission invariants:'
            Write-Host $log.FullName
            return
        }
        if ($content -match 'ENV:\[invalid\].*Error - Cannot send request') {
            Write-Host 'BACKEND STARTUP BLOCKED before mission loading. Preserve this log:'
            Write-Host $log.FullName
            return
        }
        if ($content -match '\[TB_CC_M4XP\] INIT_FAILED|\[TB_CC_M4_4\] INVALID') {
            Write-Host 'MISSION STARTUP FAILURE. Stop later phases and inspect the first failure in:'
            Write-Host $log.FullName
            return
        }
    }
    if ($process.HasExited) { Write-Host "Server exited before online state. Profile: $serverProfile"; return }
    Start-Sleep -Milliseconds 1000
}
Write-Host "Online state not observed within $ReadyTimeoutSeconds seconds. Preserve logs at $serverProfile."
