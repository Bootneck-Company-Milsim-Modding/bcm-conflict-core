[CmdletBinding()]
param(
    [switch]$DryRun,
    [string]$ServerExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe',
    [string]$ClientExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger\ArmaReforgerSteamDiag.exe',
    [string]$ServerAddonsDirectory = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\addons',
    [string]$ClientAddonsDirectory = 'D:\SteamLibrary\steamapps\common\Arma Reforger\addons',
    [string]$AddonId = '6A6A5973FA53A5B1',
    [string]$Mission = '{A370000000000000}Missions/TB_PVE_M3_7_Playtest.conf',
    [string]$ServerAddress = '127.0.0.1',
    [ValidateRange(30, 600)][int]$ServerReadyTimeoutSeconds = 180,
    [ValidateRange(30, 600)][double]$ServerAgeBeforeClientsSeconds = 257.705,
    [ValidateRange(100, 5000)][int]$ClientSeparationMilliseconds = 779,
    [ValidateRange(0, 120)][int]$AuthenticationObservationSeconds = 30
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$workspaceAddonsDirectory = Split-Path -Parent $repoRoot
$runId = Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'
$runRoot = Join-Path $repoRoot "TestProfiles\M3_7_LocalInteractive_$runId"
$serverProfile = Join-Path $runRoot 'Server'
$clientAProfile = Join-Path $runRoot 'Client_A'
$clientBProfile = Join-Path $runRoot 'Client_B'
$serverAddons = "$ServerAddonsDirectory,$workspaceAddonsDirectory"
$clientAddons = "$ClientAddonsDirectory,$workspaceAddonsDirectory"

$serverArguments = @('-server', $Mission, '-addonsDir', $serverAddons, '-addons', $AddonId, '-profile', $serverProfile, '-maxFPS', '60', '-logLevel', 'debug')
$clientAArguments = @('-client', $ServerAddress, '-addonsDir', $clientAddons, '-addons', $AddonId, '-profile', $clientAProfile, '-logLevel', 'debug')
$clientBArguments = @('-client', $ServerAddress, '-addonsDir', $clientAddons, '-addons', $AddonId, '-profile', $clientBProfile, '-logLevel', 'debug')

function ConvertTo-ArgumentString {
    param([string[]]$Arguments)
    $rendered = foreach ($argument in $Arguments) {
        if ($argument -match '[\s,]') { '"' + $argument.Replace('"', '\"') + '"' } else { $argument }
    }
    return $rendered -join ' '
}

function Format-Command {
    param([string]$Executable, [string[]]$Arguments)
    return '"' + $Executable + '" ' + (ConvertTo-ArgumentString -Arguments $Arguments)
}

function Get-LatestConsoleLog {
    param([string]$Profile)
    $logsDirectory = Join-Path $Profile 'logs'
    if (-not (Test-Path -LiteralPath $logsDirectory)) { return $null }
    return Get-ChildItem -LiteralPath $logsDirectory -Recurse -File -Filter 'console.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
}

function Read-ProfileLog {
    param([string]$Profile)
    $log = Get-LatestConsoleLog -Profile $Profile
    if (-not $log) { return '' }
    try { return Get-Content -LiteralPath $log.FullName -Raw -ErrorAction Stop } catch { return '' }
}

function Write-AuthenticationFailureSummary {
    $summaryPath = Join-Path $runRoot 'AuthenticationFailureSummary.txt'
    $patterns = '(?i)(Steamworks|signed in|sessions.*loaded|ConnectionRequest|transport.*connect|identity=|possibly unauthorized|ticket response|player.*(connect|created)|Entered online game state|disconnect|group\s*[=:]|reason\s*[=:]|authorization|SteamAPI_Init failed)'
    $summary = [System.Collections.Generic.List[string]]::new()
    $summary.Add('Local M3.7 authentication failure — no retry performed')
    $summary.Add('Known-good M2.3 divergence boundary: after the possibly-unauthorized warning, M2.3 remained pending for 18–20 seconds, received ticket responses, and admitted both players.')
    foreach ($entry in @(
        @{ Name = 'Server'; Profile = $serverProfile },
        @{ Name = 'Client B'; Profile = $clientBProfile },
        @{ Name = 'Client A'; Profile = $clientAProfile }
    )) {
        $summary.Add('')
        $summary.Add("[$($entry.Name)]")
        $log = Get-LatestConsoleLog -Profile $entry.Profile
        if (-not $log) {
            $summary.Add('console.log not found')
            continue
        }
        $summary.Add("Log: $($log.FullName)")
        $matches = @(Get-Content -LiteralPath $log.FullName | Where-Object { $_ -match $patterns })
        if ($matches.Count -eq 0) { $summary.Add('No matching authentication/network events found.') }
        else { foreach ($line in $matches) { $summary.Add($line) } }
    }
    $summary | Set-Content -LiteralPath $summaryPath -Encoding utf8
    return $summaryPath
}

function Wait-ForServerReady {
    param([Diagnostics.Process]$Process)
    $deadline = (Get-Date).AddSeconds($ServerReadyTimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if ($Process.HasExited) { throw "ServerDiag exited before readiness. Logs: $serverProfile" }
        $text = Read-ProfileLog -Profile $serverProfile
        if ($text -match 'Entered online game state\.' -and
            $text -match 'Starting RPL server, listening on address .*:2001' -and
            $text -match '\[TB_PVE_M3_7\] telemetry-start authority=1 readOnly=1 intervalSeconds=10 proofActions=0') { return }
        Start-Sleep -Milliseconds 500
    }
    throw "ServerDiag did not reach the positive M3.7 ready state within $ServerReadyTimeoutSeconds seconds. Logs: $serverProfile"
}

foreach ($requiredFile in @($ServerExecutable, $ClientExecutable)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) { throw "Required executable not found: $requiredFile" }
}
foreach ($requiredDirectory in @($ServerAddonsDirectory, $ClientAddonsDirectory, $workspaceAddonsDirectory)) {
    if (-not (Test-Path -LiteralPath $requiredDirectory -PathType Container)) { throw "Required addon directory not found: $requiredDirectory" }
}

if (@(Get-Process -Name 'steam' -ErrorAction SilentlyContinue).Count -eq 0) {
    throw 'Steam is not running. Start Steam and sign into the normal account before using this launcher.'
}

$existingDevelopmentProcesses = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -in @('ArmaReforgerServerDiag.exe', 'ArmaReforgerSteamDiag.exe') })
if ($existingDevelopmentProcesses.Count -gt 0) {
    $details = ($existingDevelopmentProcesses | ForEach-Object { "$($_.Name) PID=$($_.ProcessId)" }) -join '; '
    throw "Existing diagnostic Reforger processes must be closed before this isolated run: $details"
}

Write-Host 'LOCAL M3.7 INTERACTIVE TEST'
Write-Host "Run root: $runRoot"
Write-Host "Working directory: $repoRoot"
Write-Host "Server age target before clients: $ServerAgeBeforeClientsSeconds seconds"
Write-Host "Launch order: Client B, then Client A after $ClientSeparationMilliseconds ms"
Write-Host ''
Write-Host "Server command: $(Format-Command -Executable $ServerExecutable -Arguments $serverArguments)"
Write-Host "Client B command: $(Format-Command -Executable $ClientExecutable -Arguments $clientBArguments)"
Write-Host "Client A command: $(Format-Command -Executable $ClientExecutable -Arguments $clientAArguments)"

if ($DryRun) {
    Write-Host ''
    Write-Host 'DRY RUN: PASS'
    Write-Host 'No profiles were created and no Reforger process was started.'
    exit 0
}

New-Item -ItemType Directory -Force -Path $serverProfile, $clientAProfile, $clientBProfile | Out-Null
$serverStart = Get-Date
$server = Start-Process -FilePath $ServerExecutable -ArgumentList (ConvertTo-ArgumentString $serverArguments) -WorkingDirectory $repoRoot -WindowStyle Hidden -PassThru
Wait-ForServerReady -Process $server

$targetClientLaunchTime = $serverStart.AddSeconds($ServerAgeBeforeClientsSeconds)
$remaining = ($targetClientLaunchTime - (Get-Date)).TotalMilliseconds
if ($remaining -gt 0) {
    Write-Host "Server ready. Waiting $([math]::Round($remaining / 1000, 1)) seconds to reproduce the successful M2.3 server age..."
    while ((Get-Date) -lt $targetClientLaunchTime) {
        if ($server.HasExited) { throw "ServerDiag exited during the M2.3 settle interval. Logs: $serverProfile" }
        $sleepMilliseconds = [math]::Min(1000, [math]::Max(1, [int](($targetClientLaunchTime - (Get-Date)).TotalMilliseconds)))
        Start-Sleep -Milliseconds $sleepMilliseconds
    }
}

# M2.3 submitted both clients concurrently. Its observed engine order was B first,
# then A 779 ms later. Reproduce that evidence without waiting for B authentication.
$clientBLaunch = Get-Date
$clientB = Start-Process -FilePath $ClientExecutable -ArgumentList (ConvertTo-ArgumentString $clientBArguments) -WorkingDirectory $repoRoot -PassThru
Start-Sleep -Milliseconds $ClientSeparationMilliseconds
$clientALaunch = Get-Date
$clientA = Start-Process -FilePath $ClientExecutable -ArgumentList (ConvertTo-ArgumentString $clientAArguments) -WorkingDirectory $repoRoot -PassThru

Write-Host ''
Write-Host 'LOCAL M3.7 INTERACTIVE TEST'
Write-Host "Server: RUNNING (PID $($server.Id))"
Write-Host "Client A: LAUNCHED (PID $($clientA.Id))"
Write-Host "Client B: LAUNCHED (PID $($clientB.Id))"
Write-Host "Run root: $runRoot"
Write-Host "Server age at Client B launch: $([math]::Round(($clientBLaunch - $serverStart).TotalSeconds, 3)) seconds"
Write-Host "Client B -> Client A separation: $([math]::Round(($clientALaunch - $clientBLaunch).TotalMilliseconds, 1)) ms"
Write-Host 'Both SteamDiag windows are intentionally left visible and running for manual play.'

if ($AuthenticationObservationSeconds -gt 0) {
    Write-Host "Observing authentication logs for $AuthenticationObservationSeconds seconds; the windows remain interactive..."
    $deadline = (Get-Date).AddSeconds($AuthenticationObservationSeconds)
    while ((Get-Date) -lt $deadline) {
        $serverText = Read-ProfileLog $serverProfile
        $clientAText = Read-ProfileLog $clientAProfile
        $clientBText = Read-ProfileLog $clientBProfile
        $rejectionPattern = '(?i)(group\s*[=:]\s*4.*reason\s*[=:]\s*3|reason\s*[=:]\s*3.*group\s*[=:]\s*4|authorization.*(failed|rejected)|SteamAPI_Init failed)'
        if ($serverText -match $rejectionPattern -or $clientAText -match $rejectionPattern -or $clientBText -match $rejectionPattern) {
            $failureSummary = Write-AuthenticationFailureSummary
            Write-Warning 'A SteamDiag client was rejected. No retry was started.'
            Write-Host "Authentication chronology and divergence evidence: $failureSummary"
            Write-Host "All logs are preserved under: $runRoot"
            exit 2
        }
        if ($serverText -match 'Players connected:\s*2\s*/\s*2') {
            Write-Host 'LOCAL AUTHENTICATION: BOTH CLIENTS ADMITTED'
            break
        }
        Start-Sleep -Milliseconds 500
    }
}

Write-Host 'Launcher complete. All three Reforger processes remain running.'
