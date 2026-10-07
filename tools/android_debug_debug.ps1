# SpeedClient — Debug APK: pull files/speed_startup.log via run-as (Qt Creator Debug kit).
param(
    [string]$Serial = "",
    [string[]]$AppArgs = @(),
    [switch]$NoLaunch,
    [int]$WaitSeconds = 6,
    [string]$AdbPath = "D:/APPLICATIONS/Android/SDK/platform-tools/adb.exe"
)

$Common = Join-Path $PSScriptRoot "android_startup_log_common.ps1"
. $Common

Invoke-SpeedClientAndroidStartupLog @PSBoundParameters -Profile Debug
