# File policy regression tests

The native test exercises actual file creation, first-open splitting, named-instance reuse, validation and one-shot mmap recovery. The separate expiry test uses the production two-minute delayed scan to check the configured retention and configuration snapshot.

On Linux/macOS with the repository's native build dependencies:

```sh
cmake -S tests -B build/file-policy-tests
cmake --build build/file-policy-tests --config Release
ctest --test-dir build/file-policy-tests -C Release --output-on-failure
```

For Android, configure the same project with the NDK toolchain, ABI and platform, build `xlog_file_policy_test`, then run it on a device with the matching `libc++_shared.so`. Pass `--expiry` for the delayed-cleanup test. No Android device is required to compile the test.

The Java host test runs the real SDK factories and registry with Android classes stubbed and a capture backend replacing JNI. It checks instance identity, unchanged 64-bit values, reentrant opening, failed-creation rollback and removal of the runtime setters:

```powershell
./tests/java/run.ps1
```

These changes do not add locks, allocations or filesystem queries to the log-writing path. The existing synchronous file-rotation algorithm and cleanup schedule are intentionally unchanged in this scoped migration. File size remains a soft splitting threshold. Each instance must use separate log/cache directories because cleanup is directory-wide.
