# Shared helpers for SpeedClient Android startup / crash log scripts (dot-source only).

function Invoke-SpeedClientAndroidStartupLog {
    param(
        [ValidateSet("Debug", "Release", "Auto")]
        [string]$Profile = "Auto",
        [string]$Serial = "192.168.1.3:5555",
        [string[]]$AppArgs = @(),
        [switch]$NoLaunch,
        [int]$WaitSeconds = 6,
        [string]$AdbPath = "D:/APPLICATIONS/Android/SDK/platform-tools/adb.exe"
    )

    if (-not (Test-Path $AdbPath)) {
        Write-Error "adb not found at $AdbPath"
        return
    }

    $script:Adb = $AdbPath
    $script:Serial = $Serial
    $pkg = "com.speed.client"
    $activity = "org.qtproject.qt.android.bindings.QtActivity"
    $logTag = "SpeedClient"

    function Invoke-AdbShell {
        param([string]$Command)
        $prev = $ErrorActionPreference
        $ErrorActionPreference = "Continue"
        $out = & $script:Adb -s $script:Serial shell $Command 2>&1
        $ErrorActionPreference = $prev
        return ($out | Out-String).Trim()
    }

    function Get-FilteredLogcat {
        param(
            [string]$PidFilter = "",
            [string[]]$Patterns = @(
                "SpeedClient",
                "speed_startup",
                "libSpeedClient",
                "AndroidStartupLog",
                "Qt",
                "qml",
                "FATAL",
                "Fatal signal",
                "signal 11",
                "signal 6",
                "SIGSEGV",
                "SIGABRT",
                "backtrace",
                "DEBUG",
                "libc",
                "tombstone"
            )
        )
        $prev = $ErrorActionPreference
        $ErrorActionPreference = "Continue"
        if ($PidFilter) {
            $raw = & $script:Adb -s $script:Serial logcat -d --pid=$PidFilter 2>&1
        } else {
            $raw = & $script:Adb -s $script:Serial logcat -d 2>&1
        }
        $ErrorActionPreference = $prev
        $text = ($raw | Out-String)
        $regex = ($Patterns | ForEach-Object { [regex]::Escape($_) }) -join "|"
        $lines = $text -split "`r?`n" | Where-Object { $_ -match $regex }
        return ($lines -join "`n").Trim()
    }

    function Test-StartupLogText {
        param([string]$Text)
        if ([string]::IsNullOrWhiteSpace($Text)) { return $false }
        if ($Text -match "run-as:|not debuggable|Permission denied") { return $false }
        return $Text -match "SpeedClient|speed_startup|AndroidStartupLog installed|log_path=|libSpeedClient"
    }

    function Get-ReleaseMirrorPaths {
        return @(
            "/storage/emulated/0/Android/data/com.speed.client/files/Download/SpeedClient/speed_startup.log",
            "/sdcard/Android/data/com.speed.client/files/Download/SpeedClient/speed_startup.log",
            "/sdcard/Download/SpeedClient/speed_startup.log",
            "/storage/emulated/0/Download/SpeedClient/speed_startup.log"
        )
    }

    function Read-RunAsLog {
        Invoke-AdbShell "run-as $pkg cat files/speed_startup.log"
    }

    function Read-MirrorLogs {
        param([string[]]$Paths)
        foreach ($mp in $Paths) {
            $mirrorText = Invoke-AdbShell "cat `"$mp`""
            if (Test-StartupLogText $mirrorText) {
                return @{ Text = $mirrorText; Path = $mp }
            }
            Write-Host "  (miss) $mp"
        }
        return $null
    }

    function Read-LogcatStartupLog {
        $appPid = (Invoke-AdbShell "pidof $pkg").Split(" ")[0]
        $lc = Get-FilteredLogcat -PidFilter $appPid -Patterns @(
            "SpeedClient", "speed_startup", "libSpeedClient", "AndroidStartupLog"
        )
        if (-not (Test-StartupLogText $lc)) {
            $lc = Get-FilteredLogcat -Patterns @(
                "SpeedClient", "speed_startup", "libSpeedClient", "AndroidStartupLog"
            )
        }
        if (Test-StartupLogText $lc) {
            $src = if ($appPid) { "logcat (pid $appPid)" } else { "logcat" }
            return @{ Text = $lc; Source = $src }
        }
        return $null
    }

    & $script:Adb connect $script:Serial | Out-Null

    $profileLabel = switch ($Profile) {
        "Debug" { "Debug APK (run-as)" }
        "Release" { "Release APK (external mirror)" }
        default { "Auto (run-as then mirror)" }
    }
    Write-Host "=== SpeedClient startup / crash log [$profileLabel] ==="

    $pkgDump = Invoke-AdbShell "dumpsys package $pkg"
    $isDebuggable = $pkgDump -match "DEBUGGABLE"
    Write-Host "=== installed APK ==="
    if ($isDebuggable) {
        Write-Host "DEBUGGABLE"
    } else {
        Write-Host "Release (not debuggable)"
    }

    if ($Profile -eq "Debug" -and -not $isDebuggable) {
        Write-Warning "Installed APK is not debuggable. Build/deploy Debug kit or use android_debug_release.ps1"
    }
    if ($Profile -eq "Release" -and $isDebuggable) {
        Write-Warning "Installed APK is debuggable. You can use android_debug_debug.ps1 for run-as."
    }

    if (-not $NoLaunch) {
        Write-Host "=== force-stop (best effort) ==="
        Invoke-AdbShell "am force-stop $pkg" | Out-Null
    }

    if (-not $NoLaunch) {
        Write-Host "=== clear logcat buffer (host) ==="
        $ErrorActionPreference = "Continue"
        & $script:Adb -s $script:Serial logcat -c 2>&1 | Out-Null
        $ErrorActionPreference = "Stop"

        Write-Host "=== start $pkg ($($AppArgs -join ' ')) ==="
        if ($AppArgs.Count -gt 0) {
            $argStr = ($AppArgs | ForEach-Object { $_ -replace '"', '\"' }) -join ' '
            & $script:Adb -s $script:Serial shell am start -n "$pkg/$activity" --es "applicationArguments" $argStr 2>&1
            Write-Host "Qt Creator run args if needed: $($AppArgs -join ' ')"
        } else {
            & $script:Adb -s $script:Serial shell am start -n "$pkg/$activity" 2>&1
        }
        Start-Sleep -Seconds $WaitSeconds
    }

    $logText = $null
    $source = $null
    $tryRunAs = $Profile -in @("Debug", "Auto")
    $tryMirror = $Profile -in @("Release", "Auto", "Debug")

    if ($tryRunAs) {
        Write-Host "=== speed_startup.log (run-as) ==="
        $runAsText = Read-RunAsLog
        if (Test-StartupLogText $runAsText) {
            $logText = $runAsText
            $source = "run-as"
        } elseif ($Profile -eq "Debug") {
            Write-Host $runAsText
        }
    } elseif ($Profile -eq "Release") {
        Write-Host "=== run-as (skipped for Release profile) ==="
    }

    if (-not $logText -and $tryMirror) {
        Write-Host "=== speed_startup.log (mirror) ==="
        $hit = Read-MirrorLogs -Paths (Get-ReleaseMirrorPaths)
        if ($hit) {
            $logText = $hit.Text
            $source = "mirror: $($hit.Path)"
        }
    }

    if (-not $logText) {
        Write-Host "=== speed_startup.log (host logcat) ==="
        $hit = Read-LogcatStartupLog
        if ($hit) {
            $logText = $hit.Text
            $source = $hit.Source
        }
    }

    if ($logText) {
        Write-Host "=== source: $source ==="
        Write-Host $logText
    } else {
        if ($Profile -eq "Debug") {
            Write-Warning "No startup log via run-as. Confirm Debug APK is installed and app reached main()."
        } elseif ($Profile -eq "Release") {
            Write-Warning "No startup log on mirror path. See Android/data/.../files/Download/SpeedClient/."
        } else {
            Write-Warning "No startup log from run-as, mirror, or logcat."
        }
        Write-Host "Check app runs: adb -s $Serial shell pidof $pkg"
    }

    Write-Host ""
    Write-Host "=== crash / error logcat (filtered) ==="
    $appPid = (Invoke-AdbShell "pidof $pkg").Split(" ")[0]
    $crashLog = Get-FilteredLogcat -PidFilter $appPid
    if ([string]::IsNullOrWhiteSpace($crashLog)) {
        $crashLog = Get-FilteredLogcat
    }
    if ($crashLog) {
        Write-Host $crashLog
    } else {
        Write-Host "(no matching logcat lines; try: adb -s $Serial logcat -d)"
    }

    Write-Host ""
    if ($Profile -ne "Release") {
        Write-Host "Debug:   adb -s $Serial shell run-as $pkg cat files/speed_startup.log"
    }
    if ($Profile -ne "Debug") {
        Write-Host "Release: adb -s $Serial shell cat /storage/emulated/0/Android/data/com.speed.client/files/Download/SpeedClient/speed_startup.log"
    }
    Write-Host "Logcat:  adb -s $Serial logcat -d | findstr $logTag"
}
