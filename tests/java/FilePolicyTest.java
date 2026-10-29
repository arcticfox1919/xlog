// Copyright (C) 2026. Licensed under the MIT License.
import com.tencent.mars.xlog.Log;
import com.tencent.mars.xlog.Xlog;

/** Tests real Java lifecycle and configuration APIs with only JNI replaced. */
public final class FilePolicyTest {
    private static final class Backend extends Xlog {
        int opens;
        long size;
        long age;
        boolean fail;
        boolean reenter;

        @Override public long newXlogInstance(XLogConfig config) {
            opens++;
            size = config.maxFileSizeBytes;
            age = config.maxAliveTimeSeconds;
            if (reenter) {
                try {
                    Log.openLogInstance(config);
                    throw new AssertionError("Opening instance must not be published early");
                } catch (IllegalStateException expected) { }
            }
            return fail ? 0 : 100 + opens;
        }
        @Override public void releaseXlogInstance(String prefix) { }
        @Override public int getLogLevel(long handle) { return LEVEL_INFO; }
    }

    private static void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
    }

    private static Xlog.XLogConfig config(String prefix) {
        Xlog.XLogConfig config = new Xlog.XLogConfig();
        config.nameprefix = prefix;
        config.logdir = "/logs/" + prefix;
        return config;
    }

    private static void rejected(Xlog.XLogConfig config) {
        try {
            Log.openLogInstance(config);
            throw new AssertionError("Invalid policy accepted");
        } catch (IllegalArgumentException expected) { }
    }

    public static void main(String[] args) throws Exception {
        Backend backend = new Backend();
        Log.setLogImp(backend);
        Xlog.XLogConfig config = config("policy");
        check(config.maxFileSizeBytes == 0 && config.maxAliveTimeSeconds == 864000,
                "Unexpected default policy");
        config.maxFileSizeBytes = (1L << 33) + 123;
        config.maxAliveTimeSeconds = (1L << 32) + 456;
        backend.reenter = true;
        Log.LogInstance first = Log.openLogInstance(config);
        backend.reenter = false;
        check(backend.size == config.maxFileSizeBytes && backend.age == config.maxAliveTimeSeconds,
                "64-bit fields must reach the backend unchanged");

        // Reuse must precede validation/backend work and must not change policy.
        Xlog.XLogConfig replacement = config("policy");
        replacement.logdir = null;
        replacement.maxFileSizeBytes = -1;
        replacement.maxAliveTimeSeconds = 0;
        check(Log.openLogInstance(replacement) == first && backend.opens == 1,
                "Existing instance must be returned unchanged");
        check(Log.openLogInstance(0, 0, "", "/other", "policy", 0) == first,
                "Both factory overloads must reuse the same instance");
        Log.closeLogInstance("policy");

        config = config("retry");
        config.maxFileSizeBytes = -1;
        rejected(config);
        config.maxFileSizeBytes = 0;
        config.maxAliveTimeSeconds = 86399;
        rejected(config);
        check(backend.opens == 1, "Invalid policy must not enter backend");
        config.maxAliveTimeSeconds = 86400;
        check(Log.openLogInstance(config) != null, "Failed validation must release reservation");
        Log.closeLogInstance("retry");

        config = config("zero_handle");
        backend.fail = true;
        try {
            Log.openLogInstance(config);
            throw new AssertionError("Zero native handle accepted");
        } catch (IllegalStateException expected) { }
        check(Log.getLogInstance("zero_handle") == null, "Failed instance published");
        backend.fail = false;
        check(Log.openLogInstance(config) != null, "Failed creation must be retryable");
        Log.closeLogInstance("zero_handle");

        // Default creation validates before library loading/JNI as well.
        config.maxAliveTimeSeconds = 0;
        try {
            Xlog.open(false, config);
            throw new AssertionError("Default logger accepted invalid policy");
        } catch (IllegalArgumentException expected) { }
        for (Class<?> type : new Class<?>[] {Xlog.class, Log.LogImp.class, Log.LogInstance.class}) {
            for (java.lang.reflect.Method method : type.getMethods()) {
                check(!method.getName().equals("setMaxFileSize") && !method.getName().equals("setMaxAliveTime"),
                        "Runtime file policy setter remains on " + type.getName());
            }
        }
        System.out.println("PASS: defaults, 64-bit policy, instance reuse, reentrancy, rollback and creation-only API");
    }
}
