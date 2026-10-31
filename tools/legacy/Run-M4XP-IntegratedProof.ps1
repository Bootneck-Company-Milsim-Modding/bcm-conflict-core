[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][ValidateRange(1, 6)][int]$Mode,
    [switch]$DryRun,
    [string]$ServerExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe',
    [string]$EvidenceDirectory = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'BCM-Conflict-Core-TestEvidence'),
    [ValidateRange(60, 7200)][int]$TimeoutSeconds = 3600
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$serverDirectory = Split-Path -Parent $ServerExecutable
$baseConfigPath = Join-Path $repoRoot 'ServerConfigs\M4XP_IntegratedAuthority.json'
$proofWorld = Join-Path $repoRoot "Worlds\Test\M4XP\TB_CC_M4XP_Proof_$Mode.ent"
$proofMission = Join-Path $repoRoot "Missions\Test\M4XP\TB_CC_M4XP_Proof_$Mode.conf"
$proofLayer = Join-Path $repoRoot "Worlds\Test\M4XP\TB_CC_M4XP_Proof_${Mode}_Layers\Proof.layer"
$gamePort = 2215 + $Mode
$queryPort = 17789 + $Mode
$missionId = "{A4F410000000000$Mode}Missions/Test/M4XP/TB_CC_M4XP_Proof_$Mode.conf"
$runRoot = Join-Path $EvidenceDirectory ('M4XP_Proof_Mode' + $Mode + '_' + (Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'))
$profile = Join-Path $runRoot 'Server'
$configPath = Join-Path $runRoot 'ProofServer.json'

foreach ($path in @($ServerExecutable, $baseConfigPath, $proofWorld, $proofMission, $proofLayer)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file missing: $path" }
}
$mission = Get-Content -LiteralPath $proofMission -Raw
$layer = Get-Content -LiteralPath $proofLayer -Raw
if ($mission -notmatch [regex]::Escape("{A4F400000000000$Mode}Worlds/Test/M4XP/TB_CC_M4XP_Proof_$Mode.ent")) { throw 'Proof mission does not point to its proof world.' }
if ($layer -notmatch "m_iMode $Mode") { throw 'Proof layer mode does not match the requested mode.' }
$productionLayer = Get-Content -LiteralPath (Join-Path $repoRoot 'Worlds\Production\Arland\TB_CC_Arland_Layers\A_Managers.layer') -Raw
foreach ($setting in @('m_iAuthorityProofMode 0', 'm_bEnableM3Coordinator 1', 'm_bEnableM3Garrison 1')) {
    if (-not $productionLayer.Contains($setting)) { throw "Production boundary changed: $setting" }
}
if ($productionLayer -match 'TB_CC_M4XP_IntegratedProofComponent') { throw 'Proof actor detected in production world.' }
. (Join-Path $PSScriptRoot 'Assert-NativePressureConfiguration.ps1')
$pressureDuration = Assert-NativePressureConfiguration -RepositoryRoot $repoRoot
Write-Host "Native pressure hold: $pressureDuration seconds"

$config = Get-Content -LiteralPath $baseConfigPath -Raw | ConvertFrom-Json
if ($config.game.mods.Count -ne 1 -or $config.game.mods[0].modId -ne '6A6A5973FA53A5B1') { throw 'BCM addon declaration missing from base config.' }
$config.bindPort = $gamePort
$config.publicPort = $gamePort
$config.a2s.port = $queryPort
$config.game.scenarioId = $missionId
$config.game.name = "BCM M4.XP Integrated Proof Mode $Mode"
$config.game.visible = $false
foreach ($port in @($gamePort, $queryPort)) {
    if (Get-NetUDPEndpoint -LocalPort $port -ErrorAction SilentlyContinue) { throw "UDP port $port is in use. No process will be stopped." }
}

$addonDirectories = "$serverDirectory\addons,$(Split-Path -Parent $repoRoot)"
$arguments = @('-config', $configPath, '-addonsDir', $addonDirectories, '-profile', $profile, '-maxFPS', '60', '-logLevel', 'debug')
function Quote-Argument([string]$value) {
    if ($value -match '[\s,]') { return '"' + $value.Replace('"', '\"') + '"' }
    return $value
}
$argumentString = (($arguments | ForEach-Object { Quote-Argument $_ }) -join ' ')
Write-Host "M4.XP integrated proof mode $Mode"
Write-Host "Scenario: $missionId"
Write-Host "Game/query ports: $gamePort/$queryPort UDP"
Write-Host "Profile and logs: $profile"
Write-Host "Working directory: $serverDirectory"
Write-Host "Command: `"$ServerExecutable`" $argumentString"
if ($DryRun) { Write-Host 'DRY RUN: no profile, config or process created.'; return }

if (Test-Path -LiteralPath $runRoot) { throw "Run directory already exists: $runRoot" }
New-Item -ItemType Directory -Path $profile -Force | Out-Null
$config | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $configPath -Encoding utf8
$process = Start-Process -FilePath $ServerExecutable -ArgumentList $argumentString -WorkingDirectory $serverDirectory -WindowStyle Hidden -PassThru
Write-Host "Started owned ServerDiag PID=$($process.Id). No unrelated process was stopped."
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $deadline) {
    $log = Get-ChildItem -LiteralPath (Join-Path $profile 'logs') -Recurse -File -Filter 'console.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if ($log) {
        $content = Get-Content -LiteralPath $log.FullName -Raw -ErrorAction SilentlyContinue
        if ($content -match ('\[TB_CC_M4XP_PROOF\] COMPLETE result=(PASS|FAIL|BLOCKED) mode=' + $Mode + '\b')) {
            Write-Host "PROOF RESULT: $($Matches[1])"
            Write-Host "Log: $($log.FullName)"
            if (-not $process.HasExited) { Stop-Process -Id $process.Id; Write-Host "Stopped owned test PID=$($process.Id)." }
            return
        }
        if ($content -match 'Can.t compile "Game" script module|Resource not found|Unable to initialize the game') {
            Write-Host "STARTUP/COMPILE FAILURE; inspect $($log.FullName)"
            if (-not $process.HasExited) { Stop-Process -Id $process.Id }
            return
        }
    }
    if ($process.HasExited) { Write-Host "Server exited before proof completion. Logs: $profile"; return }
    Start-Sleep -Seconds 2
}
Write-Host "PROOF TIMEOUT after $TimeoutSeconds seconds. Logs: $profile"
if (-not $process.HasExited) { Stop-Process -Id $process.Id; Write-Host "Stopped owned test PID=$($process.Id)." }
