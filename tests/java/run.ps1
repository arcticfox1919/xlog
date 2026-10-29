# Copyright (C) 2026. Licensed under the MIT License.
param([string]$Javac = 'javac', [string]$Java = 'java')
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$output = Join-Path $repoRoot 'build/java-file-policy'
$stubs = @{
    'android/content/Context.java' = 'package android.content; public class Context {}'
    'android/os/Looper.java' = 'package android.os; public class Looper { public static Looper getMainLooper() { return new Looper(); } public Thread getThread() { return Thread.currentThread(); } }'
    'android/os/Handler.java' = 'package android.os; public class Handler { public Handler(Looper looper) {} public boolean post(Runnable action) { action.run(); return true; } }'
    'android/os/Process.java' = 'package android.os; public class Process { public static int myPid() { return 1; } public static int myTid() { return 1; } }'
    'android/widget/Toast.java' = 'package android.widget; import android.content.Context; public class Toast { public static final int LENGTH_LONG = 1; public static Toast makeText(Context context, String text, int duration) { return new Toast(); } public void show() {} }'
    'android/util/Log.java' = 'package android.util; public class Log { public static int v(String tag, String text) { return 0; } public static int d(String tag, String text) { return 0; } public static int i(String tag, String text) { return 0; } public static int w(String tag, String text) { return 0; } public static int e(String tag, String text) { return 0; } public static String getStackTraceString(Throwable error) { return error.toString(); } }'
}
$sources = @()
foreach ($stub in $stubs.GetEnumerator()) {
    $path = Join-Path $output $stub.Key
    New-Item -ItemType Directory -Path (Split-Path $path) -Force | Out-Null
    [IO.File]::WriteAllText($path, $stub.Value)
    $sources += $path
}
foreach ($module in @('mars_xlog_sdk', 'mars_android_sdk')) {
    $sdk = Join-Path $repoRoot "mars/libraries/$module/src/main/java/com/tencent/mars/xlog"
    $classes = Join-Path $output $module
    New-Item -ItemType Directory -Path $classes -Force | Out-Null
    & $Javac -source 8 -target 8 -Xlint:-options -encoding UTF-8 -d $classes @sources (Join-Path $sdk 'Log.java') (Join-Path $sdk 'Xlog.java') (Join-Path $PSScriptRoot 'FilePolicyTest.java')
    if ($LASTEXITCODE -ne 0) { throw "Java compilation failed: $module" }
    Write-Output $module
    & $Java -cp $classes FilePolicyTest
    if ($LASTEXITCODE -ne 0) { throw "Java file policy tests failed: $module" }
}
