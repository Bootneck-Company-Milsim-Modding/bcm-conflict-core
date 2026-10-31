[CmdletBinding()]
param(
    [switch]$DryRun,
    [string]$ServerExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe',
    [string]$EvidenceDirectory = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'BCM-Conflict-Core-TestEvidence'),
    [ValidateRange(300, 3600)][int]$ProofTimeoutSeconds = 1800,
    [ValidateRange(30, 600)][int]$SmokeTimeoutSeconds = 240
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$serverDirectory = Split-Path -Parent $ServerExecutable
$baseline = Join-Path $repoRoot 'ServerConfigs\M4XP_FinalSmoke.json'
$productionLayer = Join-Path $repoRoot 'Worlds\Production\Arland\TB_CC_Arland_Layers\A_Managers.layer'
$proofMission = Join-Path $repoRoot 'Missions\Test\M4XP\TB_CC_M4XP_CorrectiveProof.conf'
$proofWorld = Join-Path $repoRoot 'Worlds\Test\M4XP\TB_CC_M4XP_CorrectiveProof.ent'
$proofLayer = Join-Path $repoRoot 'Worlds\Test\M4XP\TB_CC_M4XP_CorrectiveProof_Layers\Proof.layer'
$productionMissionId = '{A4F3500000000000}Missions/Production/TB_CC_Arland.conf'
$proofMissionId = '{A4F4100000000007}Missions/Test/M4XP/TB_CC_M4XP_CorrectiveProof.conf'
$runRoot = Join-Path $EvidenceDirectory ('M4XP_Corrective_' + (Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'))
$addonDirectories = "$serverDirectory\addons,$(Split-Path -Parent $repoRoot)"
$stages = @(
    [pscustomobject]@{ Name='Mode7'; Mission=$proofMissionId; GamePort=2230; QueryPort=17810; Timeout=$ProofTimeoutSeconds },
    [pscustomobject]@{ Name='ProductionSmoke'; Mission=$productionMissionId; GamePort=2231; QueryPort=17811; Timeout=$SmokeTimeoutSeconds }
)

foreach ($path in @($ServerExecutable, $baseline, $productionLayer, $proofMission, $proofWorld, $proofLayer)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file missing: $path" }
}
$configTemplate = Get-Content -LiteralPath $baseline -Raw | ConvertFrom-Json
if ($configTemplate.game.scenarioId -ne $productionMissionId -or $configTemplate.game.mods.Count -ne 1 -or $configTemplate.game.mods[0].modId -ne '6A6A5973FA53A5B1') { throw 'Production scenario or BCM mod declaration is not the expected baseline.' }
if ($configTemplate.game.visible) { throw 'The corrective suite must not advertise its servers.' }
$proofMissionText = Get-Content -LiteralPath $proofMission -Raw
$proofLayerText = Get-Content -LiteralPath $proofLayer -Raw
if ($proofMissionText -notmatch [regex]::Escape('{A4F4000000000007}Worlds/Test/M4XP/TB_CC_M4XP_CorrectiveProof.ent') -or $proofLayerText -notmatch 'm_iMode 7') { throw 'The processed Mode 7 mission/world contract is not present.' }
$productionText = Get-Content -LiteralPath $productionLayer -Raw
foreach ($setting in @('m_iAuthorityProofMode 0', 'm_bEnableM3Coordinator 1', 'm_bEnableM3Garrison 1')) {
    if (-not $productionText.Contains($setting)) { throw "Production configuration changed: $setting" }
}
if ($productionText -match 'TB_CC_M4XP_IntegratedProofComponent|TB_CC_M4XP_CORRECTIVE_PROOF') { throw 'Proof actor found in production layer.' }
. (Join-Path $PSScriptRoot 'Assert-NativePressureConfiguration.ps1')
$pressureDuration = Assert-NativePressureConfiguration -RepositoryRoot $repoRoot
Write-Host "Native pressure hold: $pressureDuration seconds"

function Assert-PortsFree([int[]]$ports) {
    $endpointLines = & netstat.exe -ano -p udp
    if ($LASTEXITCODE -ne 0 -or -not $endpointLines) { throw 'Cannot verify UDP port ownership; no process will be launched.' }
    foreach ($port in $ports) {
        if ($endpointLines | Where-Object { $_ -match ('^\s*UDP\s+\S+:' + $port + '\s+') }) { throw "UDP port $port is occupied; no process will be stopped." }
    }
}

function Quote-Argument([string]$value) {
    if ($value -match '[\s,]') { return '"' + $value.Replace('"', '\"') + '"' }
    return $value
}

function Find-ConsoleLog([string]$profile) {
    $logRoot = Join-Path $profile 'logs'
    if (-not (Test-Path -LiteralPath $logRoot)) { return $null }
    return Get-ChildItem -LiteralPath $logRoot -Recurse -File -Filter 'console.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
}

function Test-ProofEvidence([string]$content) {
    $checks = [ordered]@{
        'Fresh native state' = ($content -match '\[TB_CC_M4XP_PROOF\] START mode=7 .*ordinary=11 directGameplayOwnershipMutations=0')
        'Vanilla Military South capture' = ($content -match '\[TB_CC_M4XP_PROOF\] CaptureFinish id=ARLAND::MILITARY_SOUTH' -and $content -match '\[TB_CC_M4XP_PROOF\] vanilla-capture id=ARLAND::MILITARY_SOUTH owner=US')
        'Radio candidate decisions' = ($content -match '\[TB_CC_M4_3_RADIO\] candidate=ARLAND::AIRPORT_NORTH graphLegal=1 radioReachable=' -and $content -match '\[TB_CC_M4XP_CORRECTIVE\] post-capture candidates=')
        'Objective/task and MOB' = ($content -match '\[TB_CC_M4XP_CORRECTIVE\] post-capture .*deploymentValid=1 taskMatches=1')
        'No-reachable wait' = ($content -match '\[TB_CC_M4XP_CORRECTIVE\] radio-wait pass=1' -and $content -match '\[TB_CC_M4_3_RADIO\] objective-withheld .*retrySeconds=10')
        'Scheduled radio recovery' = ($content -match 'reason=radio-reachability-retry' -and $content -match '\[TB_CC_M4XP_CORRECTIVE\] radio-recovery pass=1')
        'Selector matrix' = (([regex]::Matches($content, '\[TB_CC_M4XP_CORRECTIVE\] selector humans=(1|2|5|8|12|16) .*pass=1')).Count -eq 6)
        'Two-axis staging' = ($content -match '\[TB_CC_M4XP_CORRECTIVE\] staging-multi-axis requested=3 distinctAtLeast2=1')
        'Safe staging degradation' = ($content -match '\[TB_CC_M4XP_CORRECTIVE\] staging-single-sector safeFallback=1')
        'Safe total staging failure' = ($content -match '\[TB_CC_M4XP_CORRECTIVE\] staging-no-valid pass=1 reason=NO_VALID_STAGING_POSITION')
        'M3.6 invariants' = ($content -match '\[TB_CC_M4XP_CORRECTIVE\] garrison-invariants pass=1 .*groups=2 units=6')
        'Mode 7 completion' = ($content -match '\[TB_CC_M4XP_PROOF\] COMPLETE result=PASS mode=7 .*physicalCaptures=1 directGameplayOwnershipMutations=0')
    }
    foreach ($name in $checks.Keys) { Write-Host ("  {0}: {1}" -f $name, $(if ($checks[$name]) { 'PASS' } else { 'FAIL' })) }
    return -not ($checks.Values -contains $false)
}

function Test-SmokeEvidence([string]$content) {
    $checks = [ordered]@{
        'Online state' = ($content -match 'Entered online game state\.')
        '12 positions / 21 edges' = ($content -match '\[TB_CC_M4_1\] READY map=ARLAND_NATIVE_FULL_01 .*positions=12 edges=21')
        '11 USSR ordinary / fresh active' = ($content -match '\[TB_CC_M4XP\] state ready=1 revision=1 outcome=1 depth=0 ordinaryUS=0 ordinaryUSSR=11 required=11 foothold=0 pressure=0')
        'Sole US MOB / no forward spawn' = ($content -match '\[TB_CC_M4_4\] deployment-policy selected=ARLAND::HQ_EAST enabledUSMobSpawnPoints=1 allForwardDisabled=1')
        'M3.2 enabled' = ($content -match '\[TB_PVE_M3_2\] initialized server=1')
        'M3.6 enabled' = ($content -match '\[TB_PVE_M3_6\] manager-start server=1')
        'Exactly one initial legal objective/task' = (([regex]::Matches($content, '\[TB_CC_M4XP_OBJECTIVE\] published desired=ARLAND::MILITARY_SOUTH revision=1 .*taskCreated=1 registeredWithTaskSystem=1')).Count -eq 1)
        'No startup pressure' = ($content -match '\[TB_CC_M4_4J\] history foothold=0 outcome=1 pressureActive=0 deadlinePresent=0')
        'No proof actor' = ($content -notmatch '\[TB_CC_M4XP_PROOF\] START|\[TB_CC_M4XP_CORRECTIVE\]')
        'No BCM script/runtime error' = ($content -notmatch '(?im)^.*SCRIPT\s+\(E\):.*TB_(CC|PVE)|\[TB_CC_M4XP\] INIT_FAILED|\[TB_CC_M4_4\] INVALID')
    }
    foreach ($name in $checks.Keys) { Write-Host ("  {0}: {1}" -f $name, $(if ($checks[$name]) { 'PASS' } else { 'FAIL' })) }
    return -not ($checks.Values -contains $false)
}

Assert-PortsFree -ports @($stages | ForEach-Object { $_.GamePort; $_.QueryPort })
Write-Host 'M4.XP FOCUSED CORRECTIVE SUITE — MANUAL UNRESTRICTED SERVERDIAG'
Write-Host "Server executable: $ServerExecutable"
Write-Host "Working directory: $serverDirectory"
Write-Host "BCM addon: 6A6A5973FA53A5B1 (game.mods; no CLI -addons)"
Write-Host "Evidence root: $runRoot"
Write-Host 'Existing Reforger processes will not be stopped.'
foreach ($stage in $stages) {
    $profile = Join-Path $runRoot $stage.Name
    $configPath = Join-Path $runRoot ($stage.Name + '.json')
    $arguments = @('-config', $configPath, '-addonsDir', $addonDirectories, '-profile', $profile, '-maxFPS', '60', '-logLevel', 'debug')
    $argumentString = ($arguments | ForEach-Object { Quote-Argument $_ }) -join ' '
    Write-Host "$($stage.Name): game/query UDP $($stage.GamePort)/$($stage.QueryPort); mission=$($stage.Mission)"
    Write-Host "  Profile/logs: $profile"
    Write-Host "  Command: `"$ServerExecutable`" $argumentString"
}
if ($DryRun) { Write-Host 'DRY RUN: no profiles, configs, or processes created.'; return }

if (Test-Path -LiteralPath $runRoot) { throw "Evidence directory already exists: $runRoot" }
New-Item -ItemType Directory -Path $runRoot -Force | Out-Null
$summary = [ordered]@{ Mode7='NOT RUN'; ProductionSmoke='NOT RUN' }
foreach ($stage in $stages) {
    Assert-PortsFree -ports @($stage.GamePort, $stage.QueryPort)
    $profile = Join-Path $runRoot $stage.Name
    $configPath = Join-Path $runRoot ($stage.Name + '.json')
    New-Item -ItemType Directory -Path $profile -Force | Out-Null
    $config = Get-Content -LiteralPath $baseline -Raw | ConvertFrom-Json
    $config.bindPort = $stage.GamePort
    $config.publicPort = $stage.GamePort
    $config.a2s.port = $stage.QueryPort
    $config.game.scenarioId = $stage.Mission
    $config.game.name = "BCM M4.XP Corrective $($stage.Name)"
    $config.game.visible = $false
    $config | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $configPath -Encoding utf8
    $arguments = @('-config', $configPath, '-addonsDir', $addonDirectories, '-profile', $profile, '-maxFPS', '60', '-logLevel', 'debug')
    $argumentString = ($arguments | ForEach-Object { Quote-Argument $_ }) -join ' '
    $process = $null
    $content = ''
    $logPath = ''
    try {
        $process = Start-Process -FilePath $ServerExecutable -ArgumentList $argumentString -WorkingDirectory $serverDirectory -WindowStyle Hidden -PassThru
        Write-Host "START $($stage.Name): owned PID=$($process.Id)"
        $deadline = (Get-Date).AddSeconds($stage.Timeout)
        $onlineAt = $null
        while ((Get-Date) -lt $deadline) {
            $log = Find-ConsoleLog $profile
            if ($log) { $logPath = $log.FullName; $content = Get-Content -LiteralPath $logPath -Raw -ErrorAction SilentlyContinue }
            if ($content -match 'ENV:\[invalid\]' -and $content -match 'Could not connect to server') { $summary[$stage.Name]='STOP: KNOWN BACKEND ENVIRONMENT BLOCK'; break }
            if ($content -match 'Can.t compile "Game" script module|Unable to initialize the game|Resource not found') { $summary[$stage.Name]='FAIL: STARTUP/RESOURCE'; break }
            if ($content -match 'Entered online game state\.' -and -not $onlineAt) { $onlineAt = Get-Date; Write-Host "ONLINE $($stage.Name): PASS" }
            if ($stage.Name -eq 'Mode7' -and $content -match '\[TB_CC_M4XP_PROOF\] COMPLETE result=(PASS|FAIL|BLOCKED) mode=7\b') {
                $summary[$stage.Name] = if (Test-ProofEvidence $content) { 'PASS' } else { 'FAIL: FOCUSED EVIDENCE' }
                break
            }
            if ($stage.Name -eq 'ProductionSmoke' -and $onlineAt -and ((Get-Date) - $onlineAt).TotalSeconds -ge 30) {
                $summary[$stage.Name] = if (Test-SmokeEvidence $content) { 'PASS' } else { 'FAIL: STARTUP EVIDENCE' }
                break
            }
            $process.Refresh()
            if ($process.HasExited) { $summary[$stage.Name]='FAIL: SERVER EXITED'; break }
            Start-Sleep -Seconds 2
        }
        if ($summary[$stage.Name] -eq 'NOT RUN') { $summary[$stage.Name]='FAIL: TIMEOUT' }
    }
    finally {
        if ($process) {
            $process.Refresh()
            if (-not $process.HasExited) { Stop-Process -Id $process.Id; Write-Host "STOP $($stage.Name): owned PID=$($process.Id) only" }
        }
        Write-Host "RESULT $($stage.Name): $($summary[$stage.Name])"
        Write-Host "LOG $($stage.Name): $logPath"
        Write-Host "PROFILE $($stage.Name): $profile"
    }
    if ($summary[$stage.Name] -ne 'PASS') { break }
}
Write-Host '--- FOCUSED CORRECTIVE SUMMARY ---'
foreach ($key in $summary.Keys) { Write-Host "$key`: $($summary[$key])" }
Write-Host 'M3.6 AUTONOMOUS COMBAT AGAINST REAL US PLAYER: HUMAN-PENDING'
Write-Host "Evidence retained: $runRoot"
