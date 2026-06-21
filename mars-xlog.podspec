Pod::Spec.new do |s|
  s.name = 'mars-xlog'
  s.version = '1.3.4'
  s.summary = 'Tencent Mars xlog binary framework for iOS.'
  s.description = 'Tencent Mars xlog binary XCFramework with Swift API and iOS device/simulator support.'
  s.homepage = 'https://github.com/arcticfox1919/xlog'
  s.license = {
    :type => 'MIT',
    :text => <<-LICENSE
Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
    LICENSE
  }
  s.author = { 'Tencent Mars' => 'https://github.com/Tencent/mars' }

  s.platform = :ios, '11.0'
  s.source = {
    :http => "https://github.com/arcticfox1919/xlog/releases/download/#{s.version}/mars-xlog.xcframework.zip",
    :sha256 => '2a288a6eec75af5a2c61e90398b5b71f282832c8d9b877f06b9210fb6002486f'
  }

  s.vendored_frameworks = 'mars.xcframework'
  s.libraries = 'c++', 'z'
  s.frameworks = 'Foundation', 'SystemConfiguration'
  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES'
  }
end
