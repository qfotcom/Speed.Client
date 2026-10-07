# SpeedClient — Release APK: pull mirrored log under Android/data/.../Download/SpeedClient/.
# Wireless adb: if script hangs on logcat, use -NoLogcatClear and/or -NoLaunch.
param(
    [string]$Serial = "",
    [string[]]$AppArgs = @(),
    [switch]$NoLaunch,
    [switch]$NoLogcatClear,
    [switch]$SkipLogcat,
    [int]$WaitSeconds = 6,
    [int]$AdbTimeoutSec = 20,
    [string]$AdbPath = "D:/APPLICATIONS/Android/SDK/platform-tools/adb.exe"
)

$Common = Join-Path $PSScriptRoot "android_startup_log_common.ps1"
. $Common

Invoke-SpeedClientAndroidStartupLog @PSBoundParameters -Profile Release
