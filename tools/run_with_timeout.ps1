param(
  [Parameter(Mandatory = $true)]
  [string]$Exe,

  [Parameter(Mandatory = $true)]
  [int]$TimeoutMs,

  [string]$WorkingDirectory = ""
)

$ErrorActionPreference = "Stop"

function Resolve-FullPath([string]$path) {
  if ([System.IO.Path]::IsPathRooted($path)) { return $path }
  return [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $path))
}

$exePath = Resolve-FullPath $Exe
if (-not (Test-Path -LiteralPath $exePath)) {
  Write-Error "run_with_timeout: exe not found: $exePath"
  exit 127
}

if ([string]::IsNullOrWhiteSpace($WorkingDirectory)) {
  $WorkingDirectory = Split-Path -Parent $exePath
} else {
  $WorkingDirectory = Resolve-FullPath $WorkingDirectory
}

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $exePath
$psi.WorkingDirectory = $WorkingDirectory
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $false
$psi.RedirectStandardError = $false

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $psi

if (-not $process.Start()) {
  Write-Error "run_with_timeout: failed to start: $exePath"
  exit 126
}

if ($process.WaitForExit($TimeoutMs)) {
  exit $process.ExitCode
}

try {
  $process.Kill($true)
} catch {
  try { $process.Kill() } catch {}
}

Write-Error "run_with_timeout: timed out after ${TimeoutMs}ms: $exePath"
exit 124

