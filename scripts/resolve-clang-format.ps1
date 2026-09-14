#Requires -Version 5.1
<#
.SYNOPSIS
    Resolves clang-format for esnacc native formatting on Windows.

.DESCRIPTION
    Search order:
    1. %APPDATA%\ClangPowerTools\LLVM\LLVM<LlvmSelectedVersion>\bin\clang-format.exe
       where <LlvmSelectedVersion> comes from %APPDATA%\ClangPowerTools\Settings.json
    2. C:\Program Files\LLVM\bin\clang-format.exe
    3. clang-format on PATH
#>
Set-StrictMode -Version Latest

function Get-ClangPowerToolsLlvmVersion
{
	if (-not $env:APPDATA)
	{
		return $null
	}

	$settingsPath = Join-Path $env:APPDATA 'ClangPowerTools\Settings.json'
	if (-not (Test-Path -LiteralPath $settingsPath))
	{
		return $null
	}

	try
	{
		$settings = Get-Content -LiteralPath $settingsPath -Raw | ConvertFrom-Json
	}
	catch
	{
		return $null
	}

	foreach ($entry in $settings)
	{
		$property = $entry.PSObject.Properties['LlvmSelectedVersion']
		if ($property -and -not [string]::IsNullOrWhiteSpace([string]$property.Value))
		{
			return [string]$property.Value
		}
	}

	return $null
}

function Get-ClangFormatVersionMajor
{
	param([Parameter(Mandatory)][string]$ClangFormatPath)

	$versionLine = & $ClangFormatPath --version 2>&1 | Select-Object -First 1
	if ($versionLine -match 'clang-format version (\d+)')
	{
		return [int]$Matches[1]
	}

	throw "Could not parse clang-format version from: $versionLine"
}

function Test-ClangFormatCandidate
{
	param(
		[Parameter(Mandatory)][string]$ClangFormatPath,
		[int]$MinimumMajorVersion = 18
	)

	if (-not (Test-Path -LiteralPath $ClangFormatPath))
	{
		return $false
	}

	try
	{
		return (Get-ClangFormatVersionMajor -ClangFormatPath $ClangFormatPath) -ge $MinimumMajorVersion
	}
	catch
	{
		return $false
	}
}

function Resolve-ClangFormat
{
	param([int]$MinimumMajorVersion = 18)

	$candidates = New-Object System.Collections.Generic.List[string]

	$llvmVersion = Get-ClangPowerToolsLlvmVersion
	if ($llvmVersion -and $env:APPDATA)
	{
		$cptLlvmBin = Join-Path $env:APPDATA "ClangPowerTools\LLVM\LLVM$llvmVersion\bin\clang-format.exe"
		$candidates.Add($cptLlvmBin)
	}

	if ($env:ProgramW6432)
	{
		$candidates.Add((Join-Path $env:ProgramW6432 'LLVM\bin\clang-format.exe'))
	}
	else
	{
		$candidates.Add('C:\Program Files\LLVM\bin\clang-format.exe')
	}

	$pathCommand = Get-Command clang-format -ErrorAction SilentlyContinue
	if ($pathCommand)
	{
		$candidates.Add($pathCommand.Source)
	}

	foreach ($candidate in ($candidates | Select-Object -Unique))
	{
		if (Test-ClangFormatCandidate -ClangFormatPath $candidate -MinimumMajorVersion $MinimumMajorVersion)
		{
			return (Resolve-Path -LiteralPath $candidate).Path
		}
	}

	$settingsHint = if ($llvmVersion) { "LLVM$llvmVersion" } else { '<LlvmSelectedVersion missing>' }
	throw @"
clang-format not found (need major version >= $MinimumMajorVersion).

Checked:
- %APPDATA%\ClangPowerTools\LLVM\$settingsHint\bin\clang-format.exe
- C:\Program Files\LLVM\bin\clang-format.exe
- PATH
"@
}

if ($MyInvocation.InvocationName -ne '.')
{
	Resolve-ClangFormat
}
