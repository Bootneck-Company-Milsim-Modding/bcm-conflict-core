function Assert-NativePressureConfiguration {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $scriptText = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'Scripts\Game\TB_CC_M4_4\TB_CC_M4_4_ArlandGraphCampaign.c') -Raw
    $defaultMatch = [regex]::Match($scriptText, '\[Attribute\(defvalue:\s*"(\d+)"[^\r\n]*\)\]\s*protected int m_iPressureDurationSeconds;')
    if (-not $defaultMatch.Success) { throw 'Native pressure duration declaration/default is unavailable.' }
    $effectiveDuration = [int]$defaultMatch.Groups[1].Value
    $layers = Get-ChildItem -LiteralPath (Join-Path $RepositoryRoot 'Worlds\Production\Arland\TB_CC_Arland_Layers') -Filter '*.layer' -File
    $layerText = ($layers | ForEach-Object { Get-Content -LiteralPath $_.FullName -Raw }) -join "`n"
    $overrides = [regex]::Matches($layerText, '(?m)^\s*m_iPressureDurationSeconds\s+(\d+)\s*$')
    if ($overrides.Count -gt 1) { throw 'Ambiguous native pressure duration overrides in production layers.' }
    if ($overrides.Count -eq 1) { $effectiveDuration = [int]$overrides[0].Groups[1].Value }
    if ($effectiveDuration -le 0) { throw 'Native pressure duration must be positive.' }
    $nativeText = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c') -Raw
    if (-not $nativeText.Contains('return Math.Max(m_iPressureDurationSeconds, 1);')) { throw 'Native pressure duration resolver is missing.' }
    return $effectiveDuration
}
