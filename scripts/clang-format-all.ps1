#Requires -Version 5.1
<#
.SYNOPSIS
    Formats all in-repo C/C++ sources with clang-format (repo root .clang-format).

.DESCRIPTION
    Skips paths listed in .clang-format-ignore and common generated/vendor trees.
    Use after changing .clang-format or to establish a formatting baseline.
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'resolve-clang-format.ps1')
$clangFormat = Resolve-ClangFormat
Write-Host "Using clang-format: $clangFormat"

function Test-FormatExcluded
{
	param([Parameter(Mandatory)][string]$FullPath)

	$relative = $FullPath.Substring($repoRoot.Length + 1).Replace('\', '/')

	if ($relative -match '^cpp-lib/jsoncpp/')
	{
		return $true
	}
	if ($relative -match '^samples/ts-microservice/.+/src/stub/')
	{
		return $true
	}
	if ($relative -eq 'compiler/core/asn_commentparser.cpp')
	{
		return $true
	}
	if ($relative -match '/node_modules/')
	{
		return $true
	}

	return $false
}

$extensions = @('*.cpp', '*.h', '*.hpp', '*.c', '*.cc', '*.cxx')
$files = Get-ChildItem -Path $repoRoot -Recurse -Include $extensions -File |
	Where-Object { -not (Test-FormatExcluded $_.FullName) } |
	Sort-Object FullName

Write-Host "Formatting $($files.Count) files..."
$index = 0
foreach ($file in $files)
{
	$index++
	if ($index % 25 -eq 0)
	{
		Write-Host "  $index / $($files.Count)"
	}
	& $clangFormat -i $file.FullName
}

Write-Host "clang-format baseline complete ($($files.Count) files)."
