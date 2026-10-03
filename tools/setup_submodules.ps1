#Requires -Version 5.1
<#
.SYNOPSIS
  初始化 Speed.Client 的 SenseDesign Git 子模块（禁止用手动复制/junction 代替）。
#>
param(
    [switch]$FetchSenseDesignBranch
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

if (-not (Test-Path (Join-Path $Root ".git"))) {
    Write-Error "Not a git repository: $Root"
}

Write-Host "==> git submodule sync --recursive"
git submodule sync --recursive

Write-Host "==> git submodule update --init third_party/SenseDesign"
git submodule update --init third_party/SenseDesign

if ($FetchSenseDesignBranch) {
    $sd = Join-Path $Root "third_party\SenseDesign"
    if (Test-Path $sd) {
        Write-Host "==> SenseDesign: checkout feature/shadcn-qml-refactor & pull"
        git -C $sd fetch origin
        git -C $sd checkout feature/shadcn-qml-refactor 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Branch feature/shadcn-qml-refactor missing locally; staying on submodule recorded commit."
        } else {
            git -C $sd pull --ff-only origin feature/shadcn-qml-refactor
        }
    }
}

Write-Host "Done. Configure: cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt6>"
