#requires -Version 5.1
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$EnvFile   = Join-Path $ScriptDir '.env'

$Owner                 = 'JonatanNevo'
$FeedUrl               = "https://nuget.pkg.github.com/$Owner/index.json"
$FeedName              = 'GitHubPackages'
$VcpkgBinarySources    = "default,readwrite;nuget,$FeedUrl,readwrite"
$VcpkgNugetRepository  = "http://github.com/$Owner/portal-framework"

function Read-DotEnv {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return }
    Write-Host "Sourcing $Path"
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^\s*#' -or [string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -match '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=(.*)$') {
            $key = $Matches[1]
            $val = $Matches[2]
            if ($val -match '^"(.*)"$' -or $val -match "^'(.*)'$") { $val = $Matches[1] }
            if (-not [Environment]::GetEnvironmentVariable($key, 'Process')) {
                [Environment]::SetEnvironmentVariable($key, $val, 'Process')
            }
        }
    }
}

function Add-EnvLine {
    param(
        [string]$Path,
        [string]$Key,
        [string]$Value,
        [switch]$Quote
    )
    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType File -Path $Path -Force | Out-Null
    }
    $content = Get-Content -LiteralPath $Path -Raw -ErrorAction SilentlyContinue
    if ($null -eq $content) { $content = '' }

    if ($content -match "(?m)^$([regex]::Escape($Key))=") { return }

    if ($content.Length -gt 0 -and -not $content.EndsWith("`n")) {
        Add-Content -LiteralPath $Path -Value ''
    }

    $line = if ($Quote) { "$Key=`"$Value`"" } else { "$Key=$Value" }
    Add-Content -LiteralPath $Path -Value $line
    Write-Host "Wrote $Key to $Path"
}

Read-DotEnv -Path $EnvFile

$gprUser = [Environment]::GetEnvironmentVariable('GPR_USERNAME', 'Process')
$gprKey  = [Environment]::GetEnvironmentVariable('GPR_KEY', 'Process')

if ($gprUser -and $gprKey) {
    Write-Host 'Using GPR_USERNAME / GPR_KEY from environment'
} else {
    Write-Host 'GPR_USERNAME / GPR_KEY are not set.'
    Write-Host "Provide a GitHub username and a personal access token with 'read:packages' and 'write:packages' scopes."

    if (-not $gprUser) {
        $gprUser = Read-Host 'GitHub username'
    }
    if (-not $gprKey) {
        $secure = Read-Host 'GitHub access token' -AsSecureString
        $bstr = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)
        try {
            $gprKey = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($bstr)
        } finally {
            [System.Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
        }
    }

    if ([string]::IsNullOrWhiteSpace($gprUser) -or [string]::IsNullOrWhiteSpace($gprKey)) {
        Write-Error 'Both GPR_USERNAME and GPR_KEY are required.'
        exit 1
    }

    Add-EnvLine -Path $EnvFile -Key 'GPR_USERNAME' -Value $gprUser
    Add-EnvLine -Path $EnvFile -Key 'GPR_KEY'      -Value $gprKey

    [Environment]::SetEnvironmentVariable('GPR_USERNAME', $gprUser, 'Process')
    [Environment]::SetEnvironmentVariable('GPR_KEY',      $gprKey,  'Process')
}

$vcpkgExe = Join-Path $ScriptDir 'vcpkg\vcpkg.exe'
if (-not (Test-Path -LiteralPath $vcpkgExe)) {
    Write-Error "vcpkg not found at $vcpkgExe. Run vcpkg\bootstrap-vcpkg.bat first."
    exit 1
}

$fetchOutput = & $vcpkgExe fetch nuget
if ($LASTEXITCODE -ne 0) {
    Write-Error 'vcpkg fetch nuget failed.'
    exit $LASTEXITCODE
}
$nuget = ($fetchOutput | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -Last 1)
if (-not $nuget) {
    Write-Error 'Could not resolve nuget.exe path from vcpkg output.'
    exit 1
}

& $nuget sources add `
    -Source $FeedUrl `
    -Name $FeedName `
    -UserName $gprUser `
    -Password $gprKey `
    -StorePasswordInClearText
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $nuget setapikey $gprKey -Source $FeedUrl
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Add-EnvLine -Path $EnvFile -Key 'VCPKG_BINARY_SOURCES'   -Value $VcpkgBinarySources   -Quote
Add-EnvLine -Path $EnvFile -Key 'VCPKG_NUGET_REPOSITORY' -Value $VcpkgNugetRepository

Write-Host 'vcpkg cache setup complete.'