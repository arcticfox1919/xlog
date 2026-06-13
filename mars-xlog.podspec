Pod::Spec.new do |s|
  s.name = 'mars-xlog'
  s.version = '1.3.1'
  s.summary = 'Tencent Mars xlog binary framework for iOS.'
  s.description = 'Tencent Mars xlog packaged as a binary XCFramework for iOS device and simulator arm64 builds.'
  s.homepage = 'https://github.com/arcticfox1919/xlog'
  s.license = { :type => 'BSD', :file => 'LICENSE' }
  s.author = { 'Tencent Mars' => 'https://github.com/Tencent/mars' }

  s.platform = :ios, '11.0'
  s.source = {
    :http => "https://github.com/arcticfox1919/xlog/releases/download/#{s.version}/mars-xlog.xcframework.zip"
  }

  s.vendored_frameworks = 'mars.xcframework'
  s.libraries = 'c++', 'z'
  s.frameworks = 'Foundation', 'SystemConfiguration'
  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES'
  }
end
