[CmdletBinding()]
param(
    [switch]$DryRun,
    [switch]$AuthTopologyTest,
    [string]$ServerExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe',
    [string]$ClientExecutable = 'D:\SteamLibrary\steamapps\common\Arma Reforger\ArmaReforgerSteamDiag.exe',
    [string]$ServerAddonsDirectory = 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\addons',
    [string]$ClientAddonsDirectory = 'D:\SteamLibrary\steamapps\common\Arma Reforger\addons',
    [string]$AddonId = '6A6A5973FA53A5B1',
    [string]$Mission = '{A242000000000000}Missions/TB_PVE_M2_4.conf',
    [string]$ServerAddress = '127.0.0.1',
    [int]$ServerReadyTimeoutSeconds = 180,
    [ValidateRange(0, 600)]
    [int]$ServerSettleSeconds = 45,
    [ValidateRange(30, 600)]
    [double]$AuthTopologyServerAgeSeconds = 257.705,
    [ValidateRange(100, 5000)]
    [int]$AuthTopologyClientSeparationMilliseconds = 779,
    [ValidateRange(30, 120)]
    [int]$AuthTopologyObservationSeconds = 30,
    [int]$ClientAuthTimeoutSeconds = 90,
    [int]$FrontierTimeoutSeconds = 240
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$workspaceAddonsDirectory = Split-Path -Parent $repoRoot
$runId = Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'
$runRoot = Join-Path $repoRoot "TestProfiles\M2_4A_Runner_$runId"
$serverProfile = Join-Path $runRoot 'Server'
$clientAProfile = Join-Path $runRoot 'Client_A'
$clientBProfile = Join-Path $runRoot 'Client_B'
# The recovered successful M2.3 launch ran every process from the addon workspace
# root. Keep that process context rather than using the executable directories.
$serverWorkingDirectory = $repoRoot
$clientWorkingDirectory = $repoRoot
$serverAddons = "$ServerAddonsDirectory,$workspaceAddonsDirectory"
$clientAddons = "$ClientAddonsDirectory,$workspaceAddonsDirectory"

$serverArguments = @(
    '-server', $Mission,
    '-addonsDir', $serverAddons,
    '-addons', $AddonId,
    '-profile', $serverProfile,
    '-maxFPS', '60',
    '-logLevel', 'debug'
)
$clientAArguments = @(
    '-client', $ServerAddress,
    '-addonsDir', $clientAddons,
    '-addons', $AddonId,
    '-profile', $clientAProfile,
    '-logLevel', 'debug'
)
$clientBArguments = @(
    '-client', $ServerAddress,
    '-addonsDir', $clientAddons,
    '-addons', $AddonId,
    '-profile', $clientBProfile,
    '-logLevel', 'debug'
)

function Format-Command {
    param([string]$Executable, [string[]]$Arguments)
    $rendered = foreach ($argument in $Arguments) {
        if ($argument -match '[\s,]') { '"' + $argument.Replace('"', '\"') + '"' } else { $argument }
    }
    return '"' + $Executable + '" ' + ($rendered -join ' ')
}

function ConvertTo-ArgumentString {
    param([string[]]$Arguments)
    $rendered = foreach ($argument in $Arguments) {
        if ($argument -match '[\s,]') { '"' + $argument.Replace('"', '\"') + '"' } else { $argument }
    }
    return $rendered -join ' '
}

function Get-LatestConsoleLog {
    param([string]$Profile)
    $logs = Join-Path $Profile 'logs'
    if (-not (Test-Path -LiteralPath $logs)) { return $null }
    return Get-ChildItem -LiteralPath $logs -Recurse -Filter 'console.log' -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
}

function Read-LogText {
    param([string]$Profile)
    $log = Get-LatestConsoleLog -Profile $Profile
    if (-not $log) { return '' }
    try { return Get-Content -LiteralPath $log.FullName -Raw -ErrorAction Stop } catch { return '' }
}

function Get-M2Snapshot {
    param([string]$Profile, [switch]$Latest)
    $log = Get-LatestConsoleLog -Profile $Profile
    if (-not $log) { return $null }

    $snapshots = [System.Collections.Generic.List[object]]::new()
    $current = $null
    foreach ($line in (Get-Content -LiteralPath $log.FullName -ErrorAction SilentlyContinue)) {
        if ($line -match '\[TB_PVE_M2_4A\] CLIENT_READY\s*$') {
            $current = [ordered]@{ Revision = $null; Frontier = $null; USLegal = $null; USSRLegal = $null; Validity = $null }
            continue
        }
        if (-not $current) { continue }
        if ($line -match '\[TB_PVE_M2_4A\] revision=(-?\d+)\s*$') { $current.Revision = [int]$Matches[1]; continue }
        if ($line -match '\[TB_PVE_M2_4A\] frontier=(-?\d+)\s*$') { $current.Frontier = [int]$Matches[1]; continue }
        if ($line -match '\[TB_PVE_M2_4A\] USLegal=(.*)\s*$') { $current.USLegal = $Matches[1].Trim(); continue }
        if ($line -match '\[TB_PVE_M2_4A\] USSRLegal=(.*)\s*$') { $current.USSRLegal = $Matches[1].Trim(); continue }
        if ($line -match '\[TB_PVE_M2_4A\] validity=(VALID|INVALID.*)\s*$') {
            $current.Validity = $Matches[1]
            if ($null -ne $current.Revision -and $null -ne $current.Frontier -and $null -ne $current.USLegal -and $null -ne $current.USSRLegal) {
                $snapshots.Add([pscustomobject]$current)
            }
            $current = $null
        }
    }
    if ($snapshots.Count -eq 0) { return $null }
    if ($Latest) { return $snapshots[$snapshots.Count - 1] }
    return $snapshots.ToArray()
}

function Test-AuthenticationBlocked {
    param([string]$ServerText, [string]$ClientText)
    if ($ServerText -match 'disconnected \(identity=0x00000000.*group=4, reason=3') { return $true }
    if ($ServerText -match 'Disconnecting identity=0x00000000: group=4 reason=3') { return $true }
    if ($ClientText -match 'disconnected \(identity=0x00000000\).*group=4, reason=3') { return $true }
    if ($ClientText -match 'SteamAPI_Init failed') { return $true }
    return $false
}

function Wait-ForCondition {
    param(
        [scriptblock]$Condition,
        [int]$TimeoutSeconds,
        [System.Diagnostics.Process[]]$Processes = @()
    )
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        $result = & $Condition
        if ($result) { return $result }
        foreach ($process in $Processes) {
            if ($process -and $process.HasExited) { return 'PROCESS_EXITED' }
        }
        Start-Sleep -Milliseconds 500
    }
    return 'TIMEOUT'
}

function Stop-OwnedProcesses {
    param([System.Diagnostics.Process[]]$Processes)
    foreach ($process in $Processes) {
        if (-not $process) { continue }
        try {
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id -Force -ErrorAction Stop
                $process.WaitForExit(5000) | Out-Null
            }
        } catch {
            Write-Warning "Could not stop owned test process $($process.Id): $($_.Exception.Message)"
        }
    }
}

function Remove-StaleRunnerProcesses {
    $allReforger = Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -in @('ArmaReforgerServerDiag.exe', 'ArmaReforgerSteamDiag.exe') }
    foreach ($candidate in $allReforger) {
        $command = [string]$candidate.CommandLine
        if ($command -and $command.Contains($repoRoot) -and $command -match 'TestProfiles\\M2_4A_Runner_') {
            Write-Host "Stopping stale runner-owned process PID $($candidate.ProcessId): $($candidate.Name)"
            Stop-Process -Id $candidate.ProcessId -Force -ErrorAction Stop
        } else {
            throw "Refusing to kill unrelated Reforger process PID $($candidate.ProcessId) ($($candidate.Name)). Close it manually and rerun."
        }
    }
}

function Start-TestProcess {
    param([string]$Executable, [string[]]$Arguments, [string]$WorkingDirectory)
    return Start-Process -FilePath $Executable -ArgumentList (ConvertTo-ArgumentString $Arguments) -WorkingDirectory $WorkingDirectory -WindowStyle Hidden -PassThru
}

function Get-AuthTopologyClientState {
    param([string]$Profile)
    $text = Read-LogText $Profile
    $identityMatch = [regex]::Match($text, 'ClientImpl event: connected \(identity=(0x[0-9A-Fa-f]+)\)')
    $connectedMatch = [regex]::Match($text, '(?m)^(?<time>\d\d:\d\d:\d\d\.\d+).*ClientImpl event: connected \(identity=(?<identity>0x[0-9A-Fa-f]+)\)')
    $disconnectMatch = [regex]::Match($text, '(?m)^(?<time>\d\d:\d\d:\d\d\.\d+).*ClientImpl event: disconnected.*(?:group=(?<group>\d+), reason=(?<reason>\d+))?')
    $worldMatch = [regex]::Match($text, '(?m)^(?<time>\d\d:\d\d:\d\d\.\d+).*Entered online game state')
    $playerMatch = [regex]::Match($text, '(?m)^(?<time>\d\d:\d\d:\d\d\.\d+).*Player created')
    $ticketMatches = [regex]::Matches($text, '(?im)^.*(?:ticket response|auth.*ticket|ticket.*response).*$')
    return [pscustomobject]@{
        SteamApiInitialized = ($text -match 'Steamworks: StartUp\(\)' -and $text -notmatch 'SteamAPI_Init failed')
        AccountSignedIn = ($text -match 'signed in')
        TransportConnected = $identityMatch.Success
        Identity = if ($identityMatch.Success) { $identityMatch.Groups[1].Value } else { '' }
        ConnectedTime = if ($connectedMatch.Success) { $connectedMatch.Groups['time'].Value } else { '' }
        Rejected = ($disconnectMatch.Success -and -not $worldMatch.Success)
        DisconnectedTime = if ($disconnectMatch.Success) { $disconnectMatch.Groups['time'].Value } else { '' }
        DisconnectGroup = if ($disconnectMatch.Success) { $disconnectMatch.Groups['group'].Value } else { '' }
        DisconnectReason = if ($disconnectMatch.Success) { $disconnectMatch.Groups['reason'].Value } else { '' }
        TicketResponseCount = $ticketMatches.Count
        PlayerCreated = $playerMatch.Success
        PlayerCreatedTime = if ($playerMatch.Success) { $playerMatch.Groups['time'].Value } else { '' }
        WorldInitialized = $worldMatch.Success
        WorldInitializedTime = if ($worldMatch.Success) { $worldMatch.Groups['time'].Value } else { '' }
    }
}

function Format-AuthTopologyClientState {
    param([string]$Name, [object]$State)
    return @(
        "$Name SteamAPI initialized: $($State.SteamApiInitialized)",
        "$Name account signed in: $($State.AccountSignedIn)",
        "$Name transport connected: $($State.TransportConnected)",
        "$Name provisional identity: $($State.Identity)",
        "$Name connected time: $($State.ConnectedTime)",
        "$Name ticket responses: $($State.TicketResponseCount)",
        "$Name player created: $($State.PlayerCreated) $($State.PlayerCreatedTime)",
        "$Name world initialized: $($State.WorldInitialized) $($State.WorldInitializedTime)",
        "$Name rejected: $($State.Rejected) time=$($State.DisconnectedTime) group=$($State.DisconnectGroup) reason=$($State.DisconnectReason)"
    )
}

function Show-Summary {
    param([hashtable]$Results, [string]$Classification, [string]$CriterionM, [string]$CriterionN)
    Write-Host ''
    Write-Host 'M2.4A NETWORK ACCEPTANCE SUMMARY'
    foreach ($name in @('SERVER START','CLIENT A AUTH','CLIENT A INITIAL PROJECTION','CLIENT A LIVE SYNC','FRONTIER 4 REACHED','CLIENT B AUTH','CLIENT B JIP','A/B CONVERGENCE')) {
        Write-Host ("{0}: {1}" -f $name, $Results[$name])
    }
    Write-Host "CLASSIFICATION: $Classification"
    Write-Host "M2.4 Criterion M: $CriterionM"
    Write-Host "M2.4 Criterion N: $CriterionN"
    Write-Host "LOG ROOT: $runRoot"
}

Write-Host 'Resolved M2.4A runner configuration:'
Write-Host "  Repository:       $repoRoot"
Write-Host "  Run root:         $runRoot"
Write-Host "  Server profile:   $serverProfile"
Write-Host "  Client A profile: $clientAProfile"
Write-Host "  Client B profile: $clientBProfile"
Write-Host "  Working directory: $repoRoot"
Write-Host "  Server settle:     $ServerSettleSeconds seconds after positive readiness"
if ($AuthTopologyTest) {
    Write-Host '  Mode:               AUTH TOPOLOGY TEST (not M2.4 acceptance)'
    Write-Host "  Target server age:  $AuthTopologyServerAgeSeconds seconds"
    Write-Host "  B-to-A separation:  $AuthTopologyClientSeparationMilliseconds milliseconds"
    Write-Host "  Observation window: $AuthTopologyObservationSeconds seconds"
}
Write-Host "  Server command:   $(Format-Command $ServerExecutable $serverArguments)"
Write-Host "  Client A command: $(Format-Command $ClientExecutable $clientAArguments)"
Write-Host "  Client B command: $(Format-Command $ClientExecutable $clientBArguments)"
Write-Host '  Topology: matched ServerDiag + two independent SteamDiag proxy clients'
if ($AuthTopologyTest) {
    Write-Host "  Launch order: server ready -> target server age -> Client B -> $AuthTopologyClientSeparationMilliseconds ms -> Client A -> authentication observation"
} else {
    Write-Host '  Launch order: server ready/settled -> Client A ready/revision 1 -> frontier 4 -> fresh Client B ready/current state'
}

if ($DryRun) {
    Write-Host 'DRY RUN: no directories created and no processes started.'
    exit 0
}

$results = @{
    'SERVER START' = 'FAIL'
    'CLIENT A AUTH' = 'FAIL'
    'CLIENT A INITIAL PROJECTION' = 'FAIL'
    'CLIENT A LIVE SYNC' = 'FAIL'
    'FRONTIER 4 REACHED' = 'FAIL'
    'CLIENT B AUTH' = 'FAIL'
    'CLIENT B JIP' = 'FAIL'
    'A/B CONVERGENCE' = 'NOT TESTED'
}
$classification = 'TEST INFRASTRUCTURE FAILED'
$criterionM = 'UNVERIFIED'
$criterionN = 'UNVERIFIED'
$ownedProcesses = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
$serverLaunchUtc = $null

try {
    foreach ($requiredPath in @($ServerExecutable, $ClientExecutable, $ServerAddonsDirectory, $ClientAddonsDirectory, $workspaceAddonsDirectory)) {
        if (-not (Test-Path -LiteralPath $requiredPath)) { throw "Required path not found: $requiredPath" }
    }
    if (-not (Get-Process -Name 'steam' -ErrorAction SilentlyContinue)) {
        $classification = 'AUTHENTICATION BLOCKED'
        throw 'Steam is not running. Start Steam and sign in before running network acceptance.'
    }
    Remove-StaleRunnerProcesses
    New-Item -ItemType Directory -Force -Path $serverProfile, $clientAProfile, $clientBProfile | Out-Null

    Write-Host 'Starting dedicated server...'
    $serverLaunchUtc = [DateTime]::UtcNow
    $serverProcess = Start-TestProcess $ServerExecutable $serverArguments $serverWorkingDirectory
    $ownedProcesses.Add($serverProcess)
    $serverReady = Wait-ForCondition -TimeoutSeconds $ServerReadyTimeoutSeconds -Processes @($serverProcess) -Condition {
        $text = Read-LogText $serverProfile
        if ($text -match '\[TB_PVE_M2_4A\] SERVER_INITIAL_READY revision=1 frontier=0 USLegal=OBJ_01_MAIDENS_BAY') { return 'PASS' }
        return $null
    }
    if ($serverReady -ne 'PASS') { throw "Server did not reach the positive revision-1 ready marker: $serverReady" }
    $results['SERVER START'] = 'PASS'

    if ($AuthTopologyTest) {
        $targetLaunchUtc = $serverLaunchUtc.AddSeconds($AuthTopologyServerAgeSeconds)
        while ([DateTime]::UtcNow -lt $targetLaunchUtc) {
            if ($serverProcess.HasExited) { throw 'Server exited before the auth-topology client launch.' }
            $remainingMs = [int][Math]::Ceiling(($targetLaunchUtc - [DateTime]::UtcNow).TotalMilliseconds)
            Start-Sleep -Milliseconds ([Math]::Min(500, [Math]::Max(1, $remainingMs)))
        }

        $clientBLaunchUtc = [DateTime]::UtcNow
        $serverAgeAtClientBLaunch = ($clientBLaunchUtc - $serverLaunchUtc).TotalSeconds
        Write-Host ("Starting auth-topology Client B at server age {0:N3}s..." -f $serverAgeAtClientBLaunch)
        $clientBProcess = Start-TestProcess $ClientExecutable $clientBArguments $clientWorkingDirectory
        $ownedProcesses.Add($clientBProcess)

        Start-Sleep -Milliseconds $AuthTopologyClientSeparationMilliseconds
        $clientALaunchUtc = [DateTime]::UtcNow
        $actualLaunchSeparationMs = ($clientALaunchUtc - $clientBLaunchUtc).TotalMilliseconds
        Write-Host ("Starting auth-topology Client A {0:N3}ms after Client B..." -f $actualLaunchSeparationMs)
        $clientAProcess = Start-TestProcess $ClientExecutable $clientAArguments $clientWorkingDirectory
        $ownedProcesses.Add($clientAProcess)

        $observationDeadline = [DateTime]::UtcNow.AddSeconds($AuthTopologyObservationSeconds)
        while ([DateTime]::UtcNow -lt $observationDeadline) {
            if ($serverProcess.HasExited) { throw 'Server exited during auth-topology observation.' }
            $stateB = Get-AuthTopologyClientState $clientBProfile
            $stateA = Get-AuthTopologyClientState $clientAProfile
            if ($stateB.Rejected -and $stateA.Rejected) { break }
            if ($stateB.WorldInitialized -and $stateA.WorldInitialized) {
                # Keep observing through the requested window so ticket responses and player creation can be recorded.
            }
            Start-Sleep -Milliseconds 500
        }

        # Allow final engine log buffers to flush before parsing the preserved evidence.
        Start-Sleep -Seconds 1
        $stateB = Get-AuthTopologyClientState $clientBProfile
        $stateA = Get-AuthTopologyClientState $clientAProfile
        $serverText = Read-LogText $serverProfile
        $playersConnecting = @([regex]::Matches($serverText, '(?m)^.*Players connecting:\s*\d+\s*/\s*\d+.*$') | ForEach-Object { $_.Value.Trim() })
        $playersConnected = @([regex]::Matches($serverText, '(?m)^.*Players connected:\s*\d+\s*/\s*\d+.*$') | ForEach-Object { $_.Value.Trim() })
        $serverAuthorizationEvents = @([regex]::Matches($serverText, '(?im)^.*(?:ServerImpl event: (?:authenticating|connected|disconnected)|possibly unauthorized client|Player connected:|Player created).*$') | ForEach-Object { $_.Value.Trim() })

        $admittedB = $stateB.PlayerCreated -or $stateB.WorldInitialized
        $admittedA = $stateA.PlayerCreated -or $stateA.WorldInitialized
        if ($admittedB -and $admittedA) { $authTopologyResult = 'AUTH_TOPOLOGY_PASS' }
        elseif ($admittedB -or $admittedA) { $authTopologyResult = 'PARTIAL' }
        else { $authTopologyResult = 'AUTHENTICATION_BLOCKED' }

        $reportLines = [System.Collections.Generic.List[string]]::new()
        $reportLines.Add('M2.4A AUTH TOPOLOGY TEST')
        $reportLines.Add('This is not M2.4 acceptance; criteria M/N are not evaluated.')
        $reportLines.Add("Server launch UTC: $($serverLaunchUtc.ToString('o'))")
        $reportLines.Add(("Server age at Client B launch: {0:N3} seconds" -f $serverAgeAtClientBLaunch))
        $reportLines.Add("Client B launch UTC: $($clientBLaunchUtc.ToString('o'))")
        $reportLines.Add("Client A launch UTC: $($clientALaunchUtc.ToString('o'))")
        $reportLines.Add(("Actual B-to-A launch separation: {0:N3} milliseconds" -f $actualLaunchSeparationMs))
        foreach ($line in (Format-AuthTopologyClientState 'Client B' $stateB)) { $reportLines.Add($line) }
        foreach ($line in (Format-AuthTopologyClientState 'Client A' $stateA)) { $reportLines.Add($line) }
        $reportLines.Add('Server Players connecting observations:')
        if ($playersConnecting.Count) { foreach ($line in $playersConnecting) { $reportLines.Add($line) } } else { $reportLines.Add('(none)') }
        $reportLines.Add('Server Players connected observations:')
        if ($playersConnected.Count) { foreach ($line in $playersConnected) { $reportLines.Add($line) } } else { $reportLines.Add('(none)') }
        $reportLines.Add('Server authorization/admission observations:')
        if ($serverAuthorizationEvents.Count) { foreach ($line in $serverAuthorizationEvents) { $reportLines.Add($line) } } else { $reportLines.Add('(none)') }
        $reportLines.Add("AUTH TOPOLOGY RESULT: $authTopologyResult")
        $reportPath = Join-Path $runRoot 'AuthTopologyResult.txt'
        $reportLines | Set-Content -LiteralPath $reportPath -Encoding UTF8
        $reportLines | ForEach-Object { Write-Host $_ }
        Write-Host "LOG ROOT: $runRoot"
        exit $(if ($authTopologyResult -eq 'AUTH_TOPOLOGY_PASS') { 0 } elseif ($authTopologyResult -eq 'PARTIAL') { 3 } else { 2 })
    }

    if ($ServerSettleSeconds -gt 0) {
        Write-Host "Server is ready; preserving the recovered M2.3 post-ready settle interval ($ServerSettleSeconds seconds)..."
        $settleResult = Wait-ForCondition -TimeoutSeconds $ServerSettleSeconds -Processes @($serverProcess) -Condition {
            # Wait-ForCondition needs a positive condition; elapsed time is handled below.
            return $null
        }
        if ($settleResult -eq 'PROCESS_EXITED') { throw 'Server exited during the post-ready settle interval.' }
    }

    Write-Host 'Starting Client A...'
    $clientAProcess = Start-TestProcess $ClientExecutable $clientAArguments $clientWorkingDirectory
    $ownedProcesses.Add($clientAProcess)
    $clientAReady = Wait-ForCondition -TimeoutSeconds $ClientAuthTimeoutSeconds -Processes @($serverProcess, $clientAProcess) -Condition {
        $serverText = Read-LogText $serverProfile
        $clientText = Read-LogText $clientAProfile
        if (Test-AuthenticationBlocked $serverText $clientText) { return 'AUTH_BLOCKED' }
        $snapshots = @(Get-M2Snapshot $clientAProfile)
        if ($serverText -match 'Player connected: connectionID=' -and $clientText -match 'Entered online game state' -and $snapshots.Count -gt 0) { return 'PASS' }
        return $null
    }
    if ($clientAReady -eq 'PROCESS_EXITED') {
        Start-Sleep -Seconds 1
        if (Test-AuthenticationBlocked (Read-LogText $serverProfile) (Read-LogText $clientAProfile)) { $clientAReady = 'AUTH_BLOCKED' }
    }
    if ($clientAReady -eq 'AUTH_BLOCKED') {
        $classification = 'AUTHENTICATION BLOCKED'
        $results['CLIENT A AUTH'] = 'FAIL'
        Show-Summary $results $classification $criterionM $criterionN
        exit 2
    }
    if ($clientAReady -ne 'PASS') { throw "Client A was not admitted and initialized: $clientAReady" }
    $results['CLIENT A AUTH'] = 'PASS'

    $clientAInitial = @(Get-M2Snapshot $clientAProfile) | Where-Object {
        $_.Revision -eq 1 -and $_.Frontier -eq 0 -and $_.Validity -eq 'VALID' -and
        $_.USLegal -eq 'OBJ_01_MAIDENS_BAY' -and [string]::IsNullOrEmpty($_.USSRLegal)
    } | Select-Object -First 1
    if ($clientAInitial) { $results['CLIENT A INITIAL PROJECTION'] = 'PASS' }

    $liveResult = Wait-ForCondition -TimeoutSeconds $FrontierTimeoutSeconds -Processes @($serverProcess, $clientAProcess) -Condition {
        $latest = Get-M2Snapshot $clientAProfile -Latest
        if ($latest -and $latest.Revision -ge 2 -and $latest.Frontier -ge 1 -and $latest.Validity -eq 'VALID') { return 'PASS' }
        return $null
    }
    if ($liveResult -eq 'PASS') { $results['CLIENT A LIVE SYNC'] = 'PASS' }

    $midRouteResult = Wait-ForCondition -TimeoutSeconds $FrontierTimeoutSeconds -Processes @($serverProcess) -Condition {
        $text = Read-LogText $serverProfile
        if ($text -match '\[TB_PVE_M2_4A\] MID_ROUTE_READY revision=(\d+) frontier=4 USLegal=OBJ_05_LARUNS USSRLegal=OBJ_04_CHOTAIN_COASTAL revisionStable=1') { return 'PASS' }
        return $null
    }
    if ($midRouteResult -ne 'PASS') { throw "Server did not reach the authoritative frontier-4 hold: $midRouteResult" }
    $results['FRONTIER 4 REACHED'] = 'PASS'

    $serverText = Read-LogText $serverProfile
    $midRouteMatch = [regex]::Match($serverText, '\[TB_PVE_M2_4A\] MID_ROUTE_READY revision=(\d+) frontier=4 USLegal=([^\s]+) USSRLegal=([^\s]+)')
    $midRouteRevision = [int]$midRouteMatch.Groups[1].Value
    $midRouteUSLegal = $midRouteMatch.Groups[2].Value
    $midRouteUSSRLegal = $midRouteMatch.Groups[3].Value

    Write-Host "Starting fresh Client B at authoritative revision $midRouteRevision frontier 4..."
    $clientBProcess = Start-TestProcess $ClientExecutable $clientBArguments $clientWorkingDirectory
    $ownedProcesses.Add($clientBProcess)
    $clientBReady = Wait-ForCondition -TimeoutSeconds $ClientAuthTimeoutSeconds -Processes @($serverProcess, $clientBProcess) -Condition {
        $serverNow = Read-LogText $serverProfile
        $clientNow = Read-LogText $clientBProfile
        if (Test-AuthenticationBlocked $serverNow $clientNow) { return 'AUTH_BLOCKED' }
        $snapshot = Get-M2Snapshot $clientBProfile -Latest
        if ($clientNow -match 'Entered online game state' -and $snapshot) { return 'PASS' }
        return $null
    }
    if ($clientBReady -eq 'PROCESS_EXITED') {
        Start-Sleep -Seconds 1
        if (Test-AuthenticationBlocked (Read-LogText $serverProfile) (Read-LogText $clientBProfile)) { $clientBReady = 'AUTH_BLOCKED' }
    }
    if ($clientBReady -eq 'AUTH_BLOCKED') {
        $classification = 'AUTHENTICATION BLOCKED'
        $results['CLIENT B AUTH'] = 'FAIL'
        if ($results['CLIENT A AUTH'] -eq 'PASS' -and $results['CLIENT A INITIAL PROJECTION'] -eq 'PASS' -and $results['CLIENT A LIVE SYNC'] -eq 'PASS') { $criterionM = 'PASS' }
        Show-Summary $results $classification $criterionM $criterionN
        exit 2
    }
    if ($clientBReady -ne 'PASS') { throw "Client B was not admitted and initialized: $clientBReady" }
    $results['CLIENT B AUTH'] = 'PASS'

    $clientBJip = Get-M2Snapshot $clientBProfile -Latest
    if ($clientBJip -and $clientBJip.Revision -eq $midRouteRevision -and $clientBJip.Revision -ne 1 -and
        $clientBJip.Frontier -eq 4 -and $clientBJip.Validity -eq 'VALID' -and
        $clientBJip.USLegal -eq $midRouteUSLegal -and $clientBJip.USSRLegal -eq $midRouteUSSRLegal) {
        $results['CLIENT B JIP'] = 'PASS'
    }

    if (-not $clientAProcess.HasExited) {
        $clientALatest = Get-M2Snapshot $clientAProfile -Latest
        if ($clientALatest -and $clientBJip -and
            $clientALatest.Revision -eq $clientBJip.Revision -and $clientALatest.Frontier -eq $clientBJip.Frontier -and
            $clientALatest.Validity -eq $clientBJip.Validity -and $clientALatest.USLegal -eq $clientBJip.USLegal -and
            $clientALatest.USSRLegal -eq $clientBJip.USSRLegal) {
            $results['A/B CONVERGENCE'] = 'PASS'
        } else {
            $results['A/B CONVERGENCE'] = 'FAIL'
        }
    }

    if ($results['CLIENT A AUTH'] -eq 'PASS' -and $results['CLIENT A INITIAL PROJECTION'] -eq 'PASS' -and $results['CLIENT A LIVE SYNC'] -eq 'PASS') {
        $criterionM = 'PASS'
    } else {
        $criterionM = 'FAIL'
    }
    if ($results['CLIENT B AUTH'] -eq 'PASS' -and $results['CLIENT B JIP'] -eq 'PASS') {
        $criterionN = 'PASS'
    } else {
        $criterionN = 'FAIL'
    }
    if ($criterionM -eq 'PASS' -and $criterionN -eq 'PASS') { $classification = 'NETWORK ACCEPTANCE PASSED' }
    else { $classification = 'FRONTIER REPLICATION FAILED' }

    Show-Summary $results $classification $criterionM $criterionN
    if ($classification -ne 'NETWORK ACCEPTANCE PASSED') { exit 1 }
} catch {
    Write-Host ("RUNNER ERROR: {0}" -f $_.Exception.Message) -ForegroundColor Red
    Show-Summary $results $classification $criterionM $criterionN
    exit 1
} finally {
    Stop-OwnedProcesses $ownedProcesses.ToArray()
}
