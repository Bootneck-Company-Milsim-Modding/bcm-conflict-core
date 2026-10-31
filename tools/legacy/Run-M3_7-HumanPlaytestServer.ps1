[CmdletBinding()]
param(
    [string]$ServerExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServer.exe',
    [int]$MaxFPS = 60
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$serverName = 'TB PvE Conflict | M3.7 Playtest'
$gamePort = 2105
$repoRoot = Split-Path -Parent $PSScriptRoot
$serverWorkingDirectory = Split-Path -Parent $ServerExecutable
$configPath = Join-Path $repoRoot 'ServerConfigs\M3_7_HumanPlaytest.json'
$runId = Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'
$profilePath = Join-Path $repoRoot "TestProfiles\M3_7_Playtest_$runId"

if (-not (Test-Path -LiteralPath $ServerExecutable -PathType Leaf)) {
    throw "Server executable not found: $ServerExecutable"
}
if (-not (Test-Path -LiteralPath $configPath -PathType Leaf)) {
    throw "Dedicated-server config not found: $configPath"
}

$config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
if ($config.game.name -ne $serverName) { throw 'Configured server name does not match the M3.7 launch contract.' }
if ([int]$config.bindPort -ne $gamePort -or [int]$config.publicPort -ne $gamePort) { throw 'Configured bind/public ports do not match the M3.7 launch contract.' }
if ($config.game.scenarioId -ne '{A370000000000000}Missions/TB_PVE_M3_7_Playtest.conf') { throw 'Configured scenario does not match the M3.7 mission.' }
if (-not $config.game.visible) { throw 'M3.7 server-browser visibility must be enabled.' }
if ($config.game.mods.Count -ne 1 -or $config.game.mods[0].modId -ne '6A6A5973FA53A5B1') { throw 'Configured addon does not match BCM Conflict Core.' }

New-Item -ItemType Directory -Force -Path $profilePath | Out-Null

$arguments = @(
    '-config', $configPath,
    '-profile', $profilePath,
    '-maxFPS', $MaxFPS.ToString(),
    '-logLevel', 'debug'
)

function ConvertTo-ArgumentString {
    param([string[]]$Arguments)
    $rendered = foreach ($argument in $Arguments) {
        if ($argument -match '[\s,]') { '"' + $argument.Replace('"', '\"') + '"' } else { $argument }
    }
    return $rendered -join ' '
}

Write-Host "Server name: $serverName"
Write-Host "Game port: $gamePort/UDP"
Write-Host "Profile/log path: $profilePath"
Write-Host "Config: $configPath"
Write-Host "Server working directory: $serverWorkingDirectory"

$process = Start-Process `
    -FilePath $ServerExecutable `
    -ArgumentList (ConvertTo-ArgumentString $arguments) `
    -WorkingDirectory $serverWorkingDirectory `
    -WindowStyle Hidden `
    -PassThru

Write-Host "Server process started: PID $($process.Id)"
Write-Host 'The server remains running after this launcher exits. Follow console.log beneath the profile/log path.'
