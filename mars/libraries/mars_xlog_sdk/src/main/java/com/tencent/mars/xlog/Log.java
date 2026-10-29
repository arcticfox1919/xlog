package com.tencent.mars.xlog;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.widget.Toast;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;


/**
 * @author zhaoyuan zhangweizang
 */
public class Log {
    private static final String TAG = "mars.xlog.log";

    public static final int LEVEL_VERBOSE = 0;
    public static final int LEVEL_DEBUG = 1;
    public static final int LEVEL_INFO = 2;
    public static final int LEVEL_WARNING = 3;
    public static final int LEVEL_ERROR = 4;
    public static final int LEVEL_FATAL = 5;
    public static final int LEVEL_NONE = 6;

    // defaults to LEVEL_NONE
    private static int level = LEVEL_NONE;
    public static Context toastSupportContext = null;

    public interface LogImp {

        void logV(long logInstancePtr, String tag, String filename, String funcname, int linuxTid, int pid, long tid, long maintid, String log);

        void logI(long logInstancePtr, String tag, String filename, String funcname, int linuxTid, int pid, long tid, long maintid, String log);

        void logD(long logInstancePtr, String tag, String filename, String funcname, int linuxTid, int pid, long tid, long maintid, String log);

        void logW(long logInstancePtr, String tag, String filename, String funcname, int linuxTid, int pid, long tid, long maintid, String log);

        void logE(long logInstancePtr, String tag, String filename, String funcname, int linuxTid, int pid, long tid, long maintid, String log);

        void logF(long logInstancePtr, String tag, String filename, String funcname, int linuxTid, int pid, long tid, long maintid, String log);

        int getLogLevel(long logInstancePtr);

        void setAppenderMode(long logInstancePtr, int mode);

        long openLogInstance(int level, int mode, String cacheDir, String logDir, String nameprefix, int cacheDays);

        long getXlogInstance(String nameprefix);

        void releaseXlogInstance(String nameprefix);

        void appenderOpen(int level, int mode, String cacheDir, String logDir, String nameprefix, int cacheDays);

        void appenderClose();

        void appenderFlush(long logInstancePtr, boolean isSync);

        void setConsoleLogOpen(long logInstancePtr, boolean isOpen);

        String[] getFilePathFromTimeSpan(long logInstancePtr, int timeSpan, String prefix);

    }

    /** Optional capability; existing third-party LogImp implementations stay compatible. */
    public interface ConfigurableLogImp extends LogImp {
        long openLogInstance(Xlog.XLogConfig config);
    }

    private static LogImp debugLog = new LogImp() {
        private Handler handler = new Handler(Looper.getMainLooper());

        @Override
        public void logV(long logInstancePtr, String tag, String filename, String funcname, int line, int pid, long tid, long maintid, String log) {
            if (level <= LEVEL_VERBOSE) {
                android.util.Log.v(tag, log);
            }
        }

        @Override
        public void logI(long logInstancePtr, String tag, String filename, String funcname, int line, int pid, long tid, long maintid, String log) {
            if (level <= LEVEL_INFO) {
                android.util.Log.i(tag, log);
            }
        }

        @Override
        public void logD(long logInstancePtr, String tag, String filename, String funcname, int line, int pid, long tid, long maintid, String log) {
            if (level <= LEVEL_DEBUG) {
                android.util.Log.d(tag, log);
            }

        }

        @Override
        public void logW(long logInstancePtr, String tag, String filename, String funcname, int line, int pid, long tid, long maintid, String log) {
            if (level <= LEVEL_WARNING) {
                android.util.Log.w(tag, log);
            }

        }

        @Override
        public void logE(long logInstancePtr, String tag, String filename, String funcname, int line, int pid, long tid, long maintid, String log) {
            if (level <= LEVEL_ERROR) {
                android.util.Log.e(tag, log);
            }
        }


        @Override
        public void logF(long logInstancePtr, String tag, String filename, String funcname, int line, int pid, long tid, long maintid, final String log) {
            if (level > LEVEL_FATAL) {
                return;
            }
            android.util.Log.e(tag, log);
            if (toastSupportContext != null) {
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(toastSupportContext, log, Toast.LENGTH_LONG).show();
                    }
                });
            }
        }

        @Override
        public int getLogLevel(long logInstancePtr) {
            return level;
        }

        @Override
        public void setAppenderMode(long logInstancePtr, int mode) {

        }

        @Override
        public long openLogInstance(int level, int mode, String cacheDir, String logDir, String nameprefix, int cacheDays) {
            return 0;
        }

        @Override
        public long getXlogInstance(String nameprefix) {
            return 0;
        }

        @Override
        public void releaseXlogInstance(String nameprefix) {

        }

        @Override
        public void appenderOpen(int level, int mode, String cacheDir, String logDir, String nameprefix, int cacheDays) {

        }

        @Override
        public void appenderClose() {

        }

        @Override
        public void appenderFlush(long logInstancePtr, boolean isSync) {
        }

        @Override
        public void setConsoleLogOpen(long logInstancePtr, boolean isOpen) {

        }

        @Override
        public String[] getFilePathFromTimeSpan(long logInstancePtr, int timeSpan, String prefix) {
            return new String[0];
        }

    };

    private static LogImp logImp = debugLog;

    /**
     * Selects the backend for global calls and future instances. Existing named
     * instances retain their creator backend; close them explicitly to stop them.
     * Install the backend before starting producers.
     */
    public static void setLogImp(LogImp imp) {
        logImp = imp;
    }

    public static LogImp getImpl() {
        return logImp;
    }

    public static void appenderOpen(int level, int mode, String cacheDir, String logDir, String nameprefix, int cacheDays) {
        if (logImp != null) {
            logImp.appenderOpen(level, mode, cacheDir, logDir, nameprefix, cacheDays);
        }
    }

    public static void appenderClose() {
        LogImp backend = logImp;
        String[] prefixes;
        synchronized (sLogInstanceMap) {
            if (sClosingAllInstances || !sInstanceLifecycle.isEmpty()) {
                throw new IllegalStateException("Log instance lifecycle in progress");
            }
            prefixes = sLogInstanceMap.keySet().toArray(new String[sLogInstanceMap.size()]);
            sClosingAllInstances = true;
        }
        try {
            if (backend != null) backend.appenderClose();
            for (String prefix : prefixes) closeLogInstance(prefix, true);
        } finally {
            synchronized (sLogInstanceMap) {
                sClosingAllInstances = false;
            }
        }
    }

    public static void appenderFlush() {
        LogImp backend = logImp;
        if (backend != null) backend.appenderFlush(0, false);
        // Owned instances remain bound to their creator if logImp is replaced.
        // Iterate a snapshot; backend calls may modify the registry.
        for (LogInstance instance : snapshotLogInstances()) instance.appenderFlush();
    }

    public static void appenderFlushSync(boolean isSync) {
        if (logImp != null) {
            logImp.appenderFlush(0, isSync);

        }
    }

    public static int getLogLevel() {
        if (logImp != null) {
            return logImp.getLogLevel(0);
        }
        return LEVEL_NONE;
    }

    public static void setLevel(final int level, final boolean jni) {
        Log.level = level;
        android.util.Log.w(TAG, "new log level: " + level);

        if (jni) {
            android.util.Log.e(TAG, "no jni log level support");
        }
    }

    public static void setConsoleLogOpen(boolean isOpen) {
        if (logImp != null) {
            logImp.setConsoleLogOpen(0, isOpen);
        }
    }

    /**
     * use f(tag, format, obj) instead
     *
     * @param tag
     * @param msg
     */
    public static void f(final String tag, final String msg) {
        f(tag, msg, (Object[]) null);
    }

    /**
     * use e(tag, format, obj) instead
     *
     * @param tag
     * @param msg
     */
    public static void e(final String tag, final String msg) {
        e(tag, msg, (Object[]) null);
    }

    /**
     * use w(tag, format, obj) instead
     *
     * @param tag
     * @param msg
     */
    public static void w(final String tag, final String msg) {
        w(tag, msg, (Object[]) null);
    }

    /**
     * use i(tag, format, obj) instead
     *
     * @param tag
     * @param msg
     */
    public static void i(final String tag, final String msg) {
        i(tag, msg, (Object[]) null);
    }

    /**
     * use d(tag, format, obj) instead
     *
     * @param tag
     * @param msg
     */
    public static void d(final String tag, final String msg) {
        d(tag, msg, (Object[]) null);
    }

    /**
     * use v(tag, format, obj) instead
     *
     * @param tag
     * @param msg
     */
    public static void v(final String tag, final String msg) {
        v(tag, msg, (Object[]) null);
    }

    public static void f(String tag, final String format, final Object... obj) {
        if (logImp != null && logImp.getLogLevel(0) <= LEVEL_FATAL) {
            final String log = obj == null ? format : String.format(format, obj);
            logImp.logF(0, tag, "", "", 0, Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
        }
    }

    public static void e(String tag, final String format, final Object... obj) {
        if (logImp != null && logImp.getLogLevel(0) <= LEVEL_ERROR) {
            String log = obj == null ? format : String.format(format, obj);
            if (log == null) {
                log = "";
            }
            logImp.logE(0, tag, "", "", 0, Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
        }
    }

    public static void w(String tag, final String format, final Object... obj) {
        if (logImp != null && logImp.getLogLevel(0) <= LEVEL_WARNING) {
            String log = obj == null ? format : String.format(format, obj);
            if (log == null) {
                log = "";
            }
            logImp.logW(0, tag, "", "", 0, Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
        }
    }

    public static void i(String tag, final String format, final Object... obj) {
        if (logImp != null && logImp.getLogLevel(0) <= LEVEL_INFO) {
            String log = obj == null ? format : String.format(format, obj);
            if (log == null) {
                log = "";
            }
            logImp.logI(0, tag, "", "", 0, Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
        }
    }

    public static void d(String tag, final String format, final Object... obj) {
        if (logImp != null && logImp.getLogLevel(0) <= LEVEL_DEBUG) {
            String log = obj == null ? format : String.format(format, obj);
            if (log == null) {
                log = "";
            }
            logImp.logD(0, tag, "", "", 0, Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
        }
    }

    public static void v(String tag, final String format, final Object... obj) {
        if (logImp != null && logImp.getLogLevel(0) <= LEVEL_VERBOSE) {
            String log = obj == null ? format : String.format(format, obj);
            if (log == null) {
                log = "";
            }
            logImp.logV(0, tag, "", "", 0, Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
        }
    }

    public static void printErrStackTrace(String tag, Throwable tr, final String format, final Object... obj) {
        if (logImp != null && logImp.getLogLevel(0) <= LEVEL_ERROR) {
            String log = obj == null ? format : String.format(format, obj);
            if (log == null) {
                log = "";
            }
            log += "  " + android.util.Log.getStackTraceString(tr);
            logImp.logE(0, tag, "", "", 0, Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
        }
    }

    // private static final String SYS_INFO;

    // static {
    //     final StringBuilder sb = new StringBuilder();
    //     try {
    //         sb.append("VERSION.RELEASE:[" + android.os.Build.VERSION.RELEASE);
    //         sb.append("] VERSION.CODENAME:[" + android.os.Build.VERSION.CODENAME);
    //         sb.append("] VERSION.INCREMENTAL:[" + android.os.Build.VERSION.INCREMENTAL);
    //         sb.append("] BOARD:[" + android.os.Build.BOARD);
    //         sb.append("] DEVICE:[" + android.os.Build.DEVICE);
    //         sb.append("] DISPLAY:[" + android.os.Build.DISPLAY);
    //         sb.append("] FINGERPRINT:[" + android.os.Build.FINGERPRINT);
    //         sb.append("] HOST:[" + android.os.Build.HOST);
    //         sb.append("] MANUFACTURER:[" + android.os.Build.MANUFACTURER);
    //         sb.append("] MODEL:[" + android.os.Build.MODEL);
    //         sb.append("] PRODUCT:[" + android.os.Build.PRODUCT);
    //         sb.append("] TAGS:[" + android.os.Build.TAGS);
    //         sb.append("] TYPE:[" + android.os.Build.TYPE);
    //         sb.append("] USER:[" + android.os.Build.USER + "]");
    //     } catch (Throwable e) {
    //         e.printStackTrace();
    //     }

    //     SYS_INFO = sb.toString();
    // }

    // public static String getSysInfo() {
    //     return SYS_INFO;
    // }

    private static final Map<String, LogInstance> sLogInstanceMap = new HashMap<>();
    // Cold lifecycle state only. Never consulted by a log write. Busy prefixes
    // fail fast instead of waiting, including when a backend re-enters Java.
    private static final Set<String> sInstanceLifecycle = new HashSet<>();
    private static boolean sClosingAllInstances;

    private static LogInstance beginInstanceOpen(String prefix) {
        if (prefix == null || prefix.length() == 0) {
            throw new IllegalArgumentException("Instance prefix must not be empty");
        }
        synchronized (sLogInstanceMap) {
            if (sClosingAllInstances || sInstanceLifecycle.contains(prefix)) {
                throw new IllegalStateException("Log instance lifecycle in progress: " + prefix);
            }
            LogInstance existing = sLogInstanceMap.get(prefix);
            if (existing != null) return existing;
            sInstanceLifecycle.add(prefix);
            return null;
        }
    }

    private static void finishInstanceLifecycle(String prefix, LogInstance created) {
        synchronized (sLogInstanceMap) {
            sInstanceLifecycle.remove(prefix);
            if (created != null) sLogInstanceMap.put(prefix, created);
        }
    }

    /** Reuses a ready instance. An overlapping open/close fails fast. */
    public static LogInstance openLogInstance(int level, int mode, String cacheDir, String logDir, String nameprefix, int cacheDays) {
        LogInstance existing = beginInstanceOpen(nameprefix);
        if (existing != null) return existing;
        LogInstance created = null;
        try {
            created = new LogInstance(level, mode, cacheDir, logDir, nameprefix, cacheDays, logImp);
            return created;
        } finally {
            finishInstanceLifecycle(nameprefix, created);
        }
    }

    /**
     * Creates a named instance with all XLogConfig fields, including encryption
     * and compression. Empty pubkey disables encryption; otherwise use a valid
     * 128-hex-character secp256k1 key generated by xlog's key tool. zstd uses
     * compresslevel; the current zlib backend retains its built-in level.
     * Do not mutate config during this call. An existing prefix returns the
     * same instance without applying any of the new configuration.
     *
     * Only reservation/publication holds the registry monitor. Backend/JNI
     * calls run outside it. Ready instances alone are visible to get/list;
     * opening and closing prefixes are rejected without waiting.
     * Failure rolls back the reservation and never registers a zero handle.
     * Native owners must still serialize their own direct factory calls, and
     * stop all producers/Dart writers before closing a native appender.
     *
     * @throws IllegalArgumentException for missing fields, invalid modes/level or key format
     * @throws UnsupportedOperationException for an unsupported backend
     * @throws IllegalStateException for a busy prefix or creation failure
     */
    public static LogInstance openLogInstance(Xlog.XLogConfig config) {
        if (config == null) throw new IllegalArgumentException("Instance configuration must not be null");
        LogInstance existing = beginInstanceOpen(config.nameprefix);
        if (existing != null) return existing;
        LogInstance created = null;
        try {
            if (config.logdir == null || config.logdir.length() == 0) {
                throw new IllegalArgumentException("Instance configuration requires logdir");
            }
            if (config.level < Xlog.LEVEL_VERBOSE || config.level > Xlog.LEVEL_NONE
                    || (config.mode != Xlog.AppednerModeAsync && config.mode != Xlog.AppednerModeSync)
                    || (config.compressmode != Xlog.ZLIB_MODE && config.compressmode != Xlog.ZSTD_MODE)) {
                throw new IllegalArgumentException("Invalid log level, appender mode or compression mode");
            }
            if (config.pubkey != null && config.pubkey.length() != 0
                    && !config.pubkey.matches("[0-9a-fA-F]{128}")) {
                throw new IllegalArgumentException("pubkey must be empty or a 128-character hexadecimal public key");
            }
            Xlog.validateFilePolicy(config);
            LogImp backend = logImp;
            if (!(backend instanceof ConfigurableLogImp)) {
                throw new UnsupportedOperationException("Log backend does not support instance configuration");
            }
            created = new LogInstance(config, (ConfigurableLogImp) backend);
            return created;
        } finally {
            finishInstanceLifecycle(config.nameprefix, created);
        }
    }

    /** Stop every native/Dart producer before explicitly closing an appender. */
    public static void closeLogInstance(String prefix) {
        closeLogInstance(prefix, false);
    }

    private static void closeLogInstance(String prefix, boolean closingAll) {
        LogInstance instance;
        synchronized (sLogInstanceMap) {
            if ((!closingAll && sClosingAllInstances) || sInstanceLifecycle.contains(prefix)) {
                throw new IllegalStateException("Log instance lifecycle in progress: " + prefix);
            }
            instance = sLogInstanceMap.get(prefix);
            if (instance == null) return;
            sInstanceLifecycle.add(prefix);
            sLogInstanceMap.remove(prefix);
            instance.mLogInstancePtr = 0;
        }
        try {
            // No Java registry lock is held across native release/callbacks.
            instance.mBackend.releaseXlogInstance(prefix);
        } finally {
            finishInstanceLifecycle(prefix, null);
        }
    }

    public static LogInstance getLogInstance(String prefix) {
        synchronized (sLogInstanceMap) {
            return sLogInstanceMap.get(prefix);
        }
    }

    private static LogInstance[] snapshotLogInstances() {
        synchronized (sLogInstanceMap) {
            return sLogInstanceMap.values().toArray(new LogInstance[sLogInstanceMap.size()]);
        }
    }

    public static Set<String> getLogInstancePrefixes() {
        synchronized (sLogInstanceMap) {
            return new HashSet<>(sLogInstanceMap.keySet());
        }
    }

    public static class LogInstance {

        // A native handle must only be passed to the backend that created it.
        private final LogImp mBackend;
        private long mLogInstancePtr = 0;

        private String mPrefix = null;

        private LogInstance(Xlog.XLogConfig config, ConfigurableLogImp backend) {
            mBackend = backend;
            mPrefix = config.nameprefix;
            mLogInstancePtr = backend.openLogInstance(config);
            if (mLogInstancePtr == 0) {
                throw new IllegalStateException("Native log instance creation failed: " + mPrefix);
            }
        }

        private LogInstance(int level, int mode, String cacheDir, String logDir, String nameprefix, int cacheDays, LogImp backend) {
            if (backend == null) throw new IllegalStateException("Log backend is not installed");
            mBackend = backend;
            mPrefix = nameprefix;
            mLogInstancePtr = backend.openLogInstance(level, mode, cacheDir, logDir, nameprefix, cacheDays);
            if (mLogInstancePtr == 0) {
                throw new IllegalStateException("Native log instance creation failed: " + mPrefix);
            }
        }

        public void f(String tag, final String format, final Object... obj) {
            if (getLogLevel() <= LEVEL_FATAL && mLogInstancePtr != 0) {
                final String log = obj == null ? format : String.format(format, obj);
                mBackend.logF(mLogInstancePtr, tag, "", "", Process.myTid(), Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
            }
        }

        public void e(String tag, final String format, final Object... obj) {
            if (getLogLevel() <= LEVEL_ERROR && mLogInstancePtr != 0) {
                String log = obj == null ? format : String.format(format, obj);
                if (log == null) {
                    log = "";
                }
                mBackend.logE(mLogInstancePtr, tag, "", "", Process.myTid(), Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
            }
        }

        public void w(String tag, final String format, final Object... obj) {
            if (getLogLevel() <= LEVEL_WARNING && mLogInstancePtr != 0) {
                String log = obj == null ? format : String.format(format, obj);
                if (log == null) {
                    log = "";
                }
                mBackend.logW(mLogInstancePtr, tag, "", "", Process.myTid(), Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
            }
        }

        public void i(String tag, final String format, final Object... obj) {
            if (getLogLevel() <= LEVEL_INFO && mLogInstancePtr != 0) {
                String log = obj == null ? format : String.format(format, obj);
                if (log == null) {
                    log = "";
                }
                mBackend.logI(mLogInstancePtr, tag, "", "", Process.myTid(), Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
            }
        }

        public void d(String tag, final String format, final Object... obj) {
            if (getLogLevel() <= LEVEL_DEBUG && mLogInstancePtr != 0) {
                String log = obj == null ? format : String.format(format, obj);
                if (log == null) {
                    log = "";
                }
                mBackend.logD(mLogInstancePtr, tag, "", "", Process.myTid(), Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
            }
        }

        public void v(String tag, final String format, final Object... obj) {
            if (getLogLevel() <= LEVEL_VERBOSE && mLogInstancePtr != 0) {
                String log = obj == null ? format : String.format(format, obj);
                if (log == null) {
                    log = "";
                }
                mBackend.logV(mLogInstancePtr, tag, "", "", Process.myTid(), Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
            }
        }


        public void printErrStackTrace(String tag, Throwable tr, final String format, final Object... obj) {
            if (getLogLevel() <= LEVEL_ERROR && mLogInstancePtr != 0) {
                String log = obj == null ? format : String.format(format, obj);
                if (log == null) {
                    log = "";
                }
                log += "  " + android.util.Log.getStackTraceString(tr);
                mBackend.logE(mLogInstancePtr, tag, "", "", Process.myTid(), Process.myPid(), Thread.currentThread().getId(), Looper.getMainLooper().getThread().getId(), log);
            }
        }


        public void appenderFlush() {
            if (mLogInstancePtr != 0) {
                mBackend.appenderFlush(mLogInstancePtr, false);
            }
        }

        public void appenderFlushSync() {
            if (mLogInstancePtr != 0) {
                mBackend.appenderFlush(mLogInstancePtr, true);
            }
        }

        public int getLogLevel() {
            if (mLogInstancePtr != 0) {
                return mBackend.getLogLevel(mLogInstancePtr);
            }
            return LEVEL_NONE;
        }

        public void setConsoleLogOpen(boolean isOpen) {
            if (mLogInstancePtr != 0) {
                mBackend.setConsoleLogOpen(mLogInstancePtr, isOpen);
            }
        }

        /**
         * Returns this instance's log file paths over the last {@code timeSpan}
         * days counting back from today (0 = today only).
         */
        public String[] getFilesFromTimeSpan(int timeSpan) {
            if (mLogInstancePtr != 0) {
                return mBackend.getFilePathFromTimeSpan(mLogInstancePtr, timeSpan, mPrefix);
            }
            return new String[0];
        }


    }
}
