# SpeedClient — Release APK: pull mirrored log under Android/data/.../Download/SpeedClient/.
param(
    [string]$Serial = "192.168.1.3:5555",
    [string[]]$AppArgs = @(),
    [switch]$NoLaunch,
    [int]$WaitSeconds = 6,
    [string]$AdbPath = "D:/APPLICATIONS/Android/SDK/platform-tools/adb.exe"
)

$Common = Join-Path $PSScriptRoot "android_startup_log_common.ps1"
. $Common

Invoke-SpeedClientAndroidStartupLog @PSBoundParameters -Profile Release
