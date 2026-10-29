# mars-xlog for Swift

`mars-xlog` exposes a Swift-compatible Objective-C API from the binary `mars` module. No bridging header or C++ interoperability setting is required.

## Build the XCFramework

```bash
cd mars
python3 build_ios.py xlog-xcframework 1.4.0
```

The command creates:

- `cmake_build/iOS/iOS.out/mars.xcframework`
- `cmake_build/iOS/iOS.out/mars-xlog.xcframework.zip`

The XCFramework contains iOS arm64 and iOS Simulator arm64/x86_64 slices.

## Swift usage

```swift
import mars

let logDirectory = FileManager.default.urls(
    for: .documentDirectory,
    in: .userDomainMask
)[0].appendingPathComponent("xlog").path

let configuration = MarsXLogConfiguration(
    logDirectory: logDirectory,
    namePrefix: "example"
)
configuration.level = .debug
configuration.mode = .async
configuration.compressMode = .zlib
configuration.compressLevel = 6
configuration.maxFileSizeBytes = 8 * 1024 * 1024
configuration.maxAliveTimeSeconds = 7 * 24 * 60 * 60

guard MarsXLog.open(with: configuration) else {
    fatalError("Unable to create the xlog directory")
}

MarsXLog.setConsoleLogEnabled(true)
MarsXLog.info("network", message: "request started")
MarsXLog.log(
    with: .error,
    tag: "network",
    message: "request failed",
    file: #fileID,
    function: #function,
    line: #line
)

MarsXLog.flushSync()
MarsXLog.close()
```

## Multiple log instances

```swift
let paymentConfiguration = MarsXLogConfiguration(
    logDirectory: URL(fileURLWithPath: logDirectory).appendingPathComponent("payment").path,
    namePrefix: "payment"
)
paymentConfiguration.level = .info
paymentConfiguration.maxFileSizeBytes = 8 * 1024 * 1024
paymentConfiguration.maxAliveTimeSeconds = 7 * 24 * 60 * 60

if let paymentLog = MarsXLog.openInstance(with: paymentConfiguration) {
    paymentLog.info("checkout", message: "payment started")
    let files = paymentLog.files(fromTimeSpan: 2)
    print(files)
    paymentLog.flushSync()
    paymentLog.close()
}
```

File size and retention are creation-only settings for both the default logger and named instances; there are no runtime setters. Omitted settings use a size threshold of `0` (no size-based splitting) and a retention threshold of 10 days. Negative sizes or retention below one day fail creation. The size is a splitting threshold, not a hard limit; expiry is based on last modification and cleanup is delayed after opening.

Opening an existing name prefix returns the same instance and ignores the new configuration. Use a dedicated log directory and cache directory for each instance: cleanup scans the entire directory, without filtering by prefix.

`MarsXLog` also provides verbose, debug, warning, error and fatal methods, console output control, synchronous/asynchronous flush, instance lookup and instance release.

## CocoaPods release

Before publishing version 1.4.0, upload `mars-xlog.xcframework.zip` to the GitHub release named `1.4.0`, then run:

```bash
pod spec lint mars-xlog.podspec --allow-warnings
```
