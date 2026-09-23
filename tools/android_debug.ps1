# SpeedClient — Auto: run-as then mirror (Debug or Release APK).
# Prefer android_debug_debug.ps1 or android_debug_release.ps1 to match your Qt kit.
param(
    [string]$Serial = "192.168.1.3:5555",
    [string[]]$AppArgs = @(),
    [switch]$NoLaunch,
    [int]$WaitSeconds = 6,
    [string]$AdbPath = "D:/APPLICATIONS/Android/SDK/platform-tools/adb.exe"
)

$Common = Join-Path $PSScriptRoot "android_startup_log_common.ps1"
. $Common

Invoke-SpeedClientAndroidStartupLog @PSBoundParameters -Profile Auto
