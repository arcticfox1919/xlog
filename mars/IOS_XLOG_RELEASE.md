# iOS xlog XCFramework Release

This release path builds only `mars/xlog` for iOS. The output XCFramework contains:

- `iphoneos` arm64
- `iphonesimulator` arm64

## Build

Run on macOS with Xcode command line tools installed:

```sh
cd mars
python3 build_ios.py xlog-xcframework 1.3.1
```

Outputs:

```text
mars/cmake_build/iOS/iOS.out/mars.xcframework
mars/cmake_build/iOS/iOS.out/mars-xlog.xcframework.zip
```

Optional architecture checks:

```sh
lipo -info cmake_build/iOS/iOS.out/iphoneos/mars.framework/mars
lipo -info cmake_build/iOS/iOS.out/iphonesimulator/mars.framework/mars
plutil -p cmake_build/iOS/iOS.out/mars.xcframework/Info.plist
```

## Publish

The podspec expects the zip to be uploaded to the GitHub Release that matches the pod version:

```text
https://github.com/arcticfox1919/xlog/releases/download/1.3.1/mars-xlog.xcframework.zip
```

Make sure tag `1.3.1` contains `mars-xlog.podspec` and the XCFramework build changes.

Create or update the release for tag `1.3.1`, then upload:

```sh
gh release create 1.3.1 cmake_build/iOS/iOS.out/mars-xlog.xcframework.zip --title 1.3.1 --notes "mars-xlog iOS XCFramework"
```

If the release already exists:

```sh
gh release upload 1.3.1 cmake_build/iOS/iOS.out/mars-xlog.xcframework.zip --clobber
```

Validate the podspec:

```sh
cd ..
pod spec lint mars-xlog.podspec --allow-warnings
```

## Consumer Dependency

Before the podspec is pushed to a specs repo, consumers can point to this repository's podspec:

```ruby
pod 'mars-xlog', :podspec => 'https://raw.githubusercontent.com/arcticfox1919/xlog/1.3.1/mars-xlog.podspec'
```

After publishing the podspec to a specs repo:

```ruby
pod 'mars-xlog', '1.3.1'
```
