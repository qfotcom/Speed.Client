# Shared helpers for SpeedClient Android startup / crash log scripts (dot-source only).

function Invoke-SpeedClientAndroidStartupLog {
    param(
        [ValidateSet("Debug", "Release", "Auto")]
        [string]$Profile = "Auto",
        [string]$Serial = "",
        [string[]]$AppArgs = @(),
        [switch]$NoLaunch,
        [switch]$NoLogcatClear,
        [switch]$SkipLogcat,
        [int]$WaitSeconds = 6,
        [int]$AdbTimeoutSec = 25,
        [string]$AdbPath = "D:/APPLICATIONS/Android/SDK/platform-tools/adb.exe"
    )

    if (-not (Test-Path $AdbPath)) {
        Write-Error "adb not found at $AdbPath"
        return
    }

    $script:Adb = $AdbPath

    function Resolve-AdbSerial {
        param([string]$Requested)
        if (-not [string]::IsNullOrWhiteSpace($Requested)) {
            $req = $Requested.Trim()
            if ($req -match ":") {
                $psi = New-Object System.Diagnostics.ProcessStartInfo
                $psi.FileName = $script:Adb
                $psi.Arguments = "connect $req"
                $psi.RedirectStandardOutput = $true
                $psi.RedirectStandardError = $true
                $psi.UseShellExecute = $false
                $psi.CreateNoWindow = $true
                $p = [System.Diagnostics.Process]::Start($psi)
                if (-not $p.WaitForExit(8000)) {
                    try { $p.Kill($true) } catch { }
                    Write-Warning "adb connect $req timed out (8s); continuing with -s $req"
                }
            }
            return $req
        }
        $lines = & $script:Adb devices 2>&1 | Out-String
        foreach ($line in ($lines -split "`r?`n")) {
            if ($line -match "^(?<id>\S+)\s+device\s*$") {
                return $Matches["id"]
            }
        }
        return ""
    }

    $script:Serial = Resolve-AdbSerial $Serial
    if ([string]::IsNullOrWhiteSpace($script:Serial)) {
        Write-Error "No adb device. Pass -Serial <id> (e.g. FY24339119CC or 192.168.x.x:5555)."
        return
    }
    Write-Host "=== adb device: $($script:Serial) ==="
    if (-not $SkipLogcat -and $script:Serial -match ':') {
        Write-Host "=== note: wireless adb — auto -SkipLogcat (logcat often hangs). USB: -Serial <usb-id> ==="
        $SkipLogcat = $true
    }
    $pkg = "com.speed.client"
    $activity = "org.qtproject.qt.android.bindings.QtActivity"
    $logTag = "SpeedClient"

    function Invoke-AdbHost {
        param(
            [Parameter(Mandatory = $true)]
            [string[]]$AdbArgs,
            [int]$TimeoutSec = $AdbTimeoutSec
        )
        $allArgs = @("-s", $script:Serial) + $AdbArgs
        $stdoutFile = [System.IO.Path]::GetTempFileName()
        $stderrFile = [System.IO.Path]::GetTempFileName()
        try {
            $proc = Start-Process -FilePath $script:Adb -ArgumentList $allArgs `
                -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile `
                -PassThru -NoNewWindow -Wait:$false
            if (-not $proc.WaitForExit($TimeoutSec * 1000)) {
                try { $proc.Kill($true) } catch { }
                Write-Warning "adb timed out (${TimeoutSec}s): adb $($allArgs -join ' ')"
                return @{ Ok = $false; Output = "" }
            }
            $out = (Get-Content -LiteralPath $stdoutFile -Raw -ErrorAction SilentlyContinue)
            $err = (Get-Content -LiteralPath $stderrFile -Raw -ErrorAction SilentlyContinue)
            $text = ("$out`n$err").Trim()
            return @{ Ok = ($proc.ExitCode -eq 0); Output = $text }
        } finally {
            Remove-Item -LiteralPath $stdoutFile, $stderrFile -Force -ErrorAction SilentlyContinue
        }
    }

    function Invoke-AdbShell {
        param(
            [string]$Command,
            [int]$TimeoutSec = $AdbTimeoutSec
        )
        $hit = Invoke-AdbHost -AdbArgs @("shell", $Command) -TimeoutSec $TimeoutSec
        return $hit.Output
    }

    function Get-AppPid {
        $raw = Invoke-AdbShell "pidof $pkg"
        if ([string]::IsNullOrWhiteSpace($raw)) { return "" }
        foreach ($tok in ($raw -split '\s+')) {
            if ($tok -match '^\d+$') { return $tok }
        }
        return ""
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
        if ($PidFilter -and $PidFilter -match '^\d+$') {
            $hit = Invoke-AdbHost -AdbArgs @("logcat", "-d", "-t", "200", "--pid=$PidFilter") -TimeoutSec $AdbTimeoutSec
        } else {
            $hit = Invoke-AdbHost -AdbArgs @("logcat", "-d", "-t", "200") -TimeoutSec $AdbTimeoutSec
        }
        $ErrorActionPreference = $prev
        $text = if ($hit.Ok) { $hit.Output } else { "" }
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
        $appPid = Get-AppPid
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
        if (-not $NoLogcatClear) {
            Write-Host "=== clear logcat buffer (device, 8s timeout) ==="
            $clearHit = Invoke-AdbHost -AdbArgs @("shell", "logcat", "-c") -TimeoutSec 8
            if (-not $clearHit.Ok) {
                Write-Warning "logcat -c skipped or failed (common on wireless adb). Use -NoLogcatClear to skip."
            }
        } else {
            Write-Host "=== clear logcat buffer (skipped, -NoLogcatClear) ==="
        }

        Write-Host "=== start $pkg ($($AppArgs -join ' ')) ==="
        if ($AppArgs.Count -gt 0) {
            $argStr = ($AppArgs | ForEach-Object { $_ -replace '"', '\"' }) -join ' '
            $startHit = Invoke-AdbHost -AdbArgs @(
                "shell", "am start -n $pkg/$activity --es applicationArguments $argStr"
            )
            if ($startHit.Output) { Write-Host $startHit.Output }
            Write-Host "Qt Creator run args if needed: $($AppArgs -join ' ')"
        } else {
            $startHit = Invoke-AdbHost -AdbArgs @("shell", "am start -n $pkg/$activity")
            if ($startHit.Output) { Write-Host $startHit.Output }
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

    if (-not $logText -and -not $SkipLogcat) {
        Write-Host "=== speed_startup.log (host logcat) ==="
        $hit = Read-LogcatStartupLog
        if ($hit) {
            $logText = $hit.Text
            $source = $hit.Source
        }
    } elseif ($SkipLogcat) {
        Write-Host "=== host logcat (skipped, -SkipLogcat) ==="
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
        Write-Host "Check app runs: adb -s $($script:Serial) shell pidof $pkg"
    }

    Write-Host ""
    Write-Host "=== crash / error logcat (filtered) ==="
    if ($SkipLogcat) {
        Write-Host "(skipped; wireless adb logcat often hangs — omit -SkipLogcat on USB)"
    } else {
        $appPid = Get-AppPid
        $crashLog = Get-FilteredLogcat -PidFilter $appPid
        if ([string]::IsNullOrWhiteSpace($crashLog)) {
            $crashLog = Get-FilteredLogcat
        }
        if ($crashLog) {
            Write-Host $crashLog
        } else {
            Write-Host "(no matching logcat lines; try: adb -s $($script:Serial) logcat -d -t 200)"
        }
    }

    Write-Host ""
    if ($Profile -ne "Release") {
        Write-Host "Debug:   adb -s $($script:Serial) shell run-as $pkg cat files/speed_startup.log"
    }
    if ($Profile -ne "Debug") {
        Write-Host "Release: adb -s $($script:Serial) shell cat /storage/emulated/0/Android/data/com.speed.client/files/Download/SpeedClient/speed_startup.log"
    }
    Write-Host "Logcat:  adb -s $($script:Serial) logcat -d -t 200 | findstr $logTag"
}
