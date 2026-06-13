#!/usr/bin/env python3
import os
import sys
import glob
import shutil

from mars_utils import *


SCRIPT_PATH = os.path.split(os.path.realpath(__file__))[0]

BUILD_OUT_PATH = 'cmake_build/iOS'
INSTALL_PATH = BUILD_OUT_PATH + '/iOS.out'

IOS_BUILD_SIMULATOR_CMD = 'cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../ios.toolchain.cmake -DPLATFORM=SIMULATOR -DENABLE_ARC=0 -DENABLE_BITCODE=0 -DENABLE_VISIBILITY=1 && make -j8 && make install'
IOS_BUILD_OS_CMD = 'cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../ios.toolchain.cmake -DPLATFORM=OS -DENABLE_ARC=0 -DENABLE_BITCODE=0 -DENABLE_VISIBILITY=1 && make -j8 && make install'
IOS_XLOG_BUILD_CMD = 'cmake ../../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../../ios.toolchain.cmake -DPLATFORM=%s -DIOS_ARCH="arm64" -DENABLE_ARC=0 -DENABLE_BITCODE=0 -DENABLE_VISIBILITY=1 && make -j8 && make install'

GEN_IOS_OS_PROJ = 'cmake ../.. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../../ios.toolchain.cmake -DPLATFORM=OS -DIOS_ARCH="arm64" -DENABLE_ARC=0 -DENABLE_BITCODE=0 -DENABLE_VISIBILITY=1'
OPEN_SSL_ARCHS = ['x86_64', 'arm64']

XLOG_DEVICE_BUILD_PATH = BUILD_OUT_PATH + '/xlog-device-arm64'
XLOG_SIMULATOR_BUILD_PATH = BUILD_OUT_PATH + '/xlog-simulator-arm64'
XLOG_XCFRAMEWORK_OUT = INSTALL_PATH + '/mars.xcframework'
XLOG_XCFRAMEWORK_ZIP = INSTALL_PATH + '/mars-xlog.xcframework.zip'


def _quote(path):
    return '"%s"' % path


def _xlog_static_libs(build_path):
    install_path = build_path + '/iOS.out'
    return [install_path + '/libcomm.a',
            install_path + '/libmars-boost.a',
            install_path + '/libxlog.a',
            build_path + '/zstd/libzstd.a']


def _complete_static_framework(dst_framework, version):
    name = os.path.splitext(os.path.basename(dst_framework))[0]
    plist_path = dst_framework + '/Info.plist'
    modules_path = dst_framework + '/Modules'
    modulemap_path = modules_path + '/module.modulemap'

    if not os.path.exists(modules_path):
        os.makedirs(modules_path)

    with open(plist_path, 'w') as f:
        f.write('''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>{name}</string>
    <key>CFBundleIdentifier</key>
    <string>com.tencent.mars.{name}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>{name}</string>
    <key>CFBundlePackageType</key>
    <string>FMWK</string>
    <key>CFBundleShortVersionString</key>
    <string>{version}</string>
    <key>CFBundleVersion</key>
    <string>1</string>
</dict>
</plist>
'''.format(name=name, version=version))

    with open(modulemap_path, 'w') as f:
        f.write('''framework module {name} {{
    umbrella "Headers"
    export *
    module * {{ export * }}
}}
'''.format(name=name))


def _build_ios_xlog_framework(build_path, platform, dst_framework_path, version):
    clean(build_path)
    os.chdir(build_path)

    ret = os.system(IOS_XLOG_BUILD_CMD % platform)
    os.chdir(SCRIPT_PATH)
    if ret != 0:
        print('!!!!!!!!!!!build xlog %s fail!!!!!!!!!!!!!!!' % platform)
        return False

    static_lib_path = build_path + '/mars'
    static_lib_dir = os.path.dirname(static_lib_path)
    if not os.path.exists(static_lib_dir):
        os.makedirs(static_lib_dir)

    if not libtool_libs(_xlog_static_libs(build_path), static_lib_path):
        return False

    make_static_framework(static_lib_path, dst_framework_path, XLOG_COPY_HEADER_FILES, '../')
    _complete_static_framework(dst_framework_path, version)
    return True


def _create_xlog_xcframework(device_framework_path, simulator_framework_path):
    remove_if_exist(XLOG_XCFRAMEWORK_OUT)
    remove_if_exist(XLOG_XCFRAMEWORK_ZIP)

    cmd = 'xcodebuild -create-xcframework -framework %s -framework %s -output %s' % (
        _quote(device_framework_path),
        _quote(simulator_framework_path),
        _quote(XLOG_XCFRAMEWORK_OUT))
    ret = os.system(cmd)
    if ret != 0:
        print('!!!!!!!!!!!create xcframework fail, cmd:%s!!!!!!!!!!!!!!!' % cmd)
        return False

    shutil.make_archive(XLOG_XCFRAMEWORK_ZIP[:-4], 'zip', INSTALL_PATH, 'mars.xcframework')
    return True


def build_ios(tag=''):
    gen_mars_revision_file('comm', tag)
    
    clean(BUILD_OUT_PATH)
    os.chdir(BUILD_OUT_PATH)
    
    ret = os.system(IOS_BUILD_OS_CMD)
    os.chdir(SCRIPT_PATH)
    if ret != 0:
        print('!!!!!!!!!!!build os fail!!!!!!!!!!!!!!!')
        return False

    libtool_os_dst_lib = INSTALL_PATH + '/os'
    libtool_src_lib = glob.glob(INSTALL_PATH + '/*.a')
    libtool_src_lib.append(BUILD_OUT_PATH + '/zstd/libzstd.a')

    if not libtool_libs(libtool_src_lib, libtool_os_dst_lib):
        return False

    clean(BUILD_OUT_PATH)
    os.chdir(BUILD_OUT_PATH)
    ret = os.system(IOS_BUILD_SIMULATOR_CMD)
    os.chdir(SCRIPT_PATH)
    if ret != 0:
        print('!!!!!!!!!!!build simulator fail!!!!!!!!!!!!!!!')
        return False
    
    libtool_simulator_dst_lib = INSTALL_PATH + '/simulator'
    if not libtool_libs(libtool_src_lib, libtool_simulator_dst_lib):
        return False

    lipo_src_libs = []
    lipo_src_libs.append(libtool_os_dst_lib)
    lipo_src_libs.append(libtool_simulator_dst_lib)
    ssl_lib = INSTALL_PATH + '/ssl'
    if not lipo_thin_libs('openssl/openssl_lib_iOS/libssl.a', ssl_lib, OPEN_SSL_ARCHS):
        return False

    crypto_lib = INSTALL_PATH + '/crypto'
    if not lipo_thin_libs('openssl/openssl_lib_iOS/libcrypto.a', crypto_lib, OPEN_SSL_ARCHS):
        return False

    lipo_src_libs.append(ssl_lib)
    lipo_src_libs.append(crypto_lib)

    lipo_dst_lib = INSTALL_PATH + '/mars'

    if not libtool_libs(lipo_src_libs, lipo_dst_lib):
        return False

    dst_framework_path = INSTALL_PATH + '/mars.framework'
    make_static_framework(lipo_dst_lib, dst_framework_path, COMM_COPY_HEADER_FILES, '../')

    print('==================Output========================')
    print(dst_framework_path)
    return True

def build_ios_xlog(tag=''):
    gen_mars_revision_file('comm', tag)
    
    clean(BUILD_OUT_PATH)
    os.chdir(BUILD_OUT_PATH)
    
    ret = os.system(IOS_BUILD_OS_CMD)
    os.chdir(SCRIPT_PATH)
    if ret != 0:
        print('!!!!!!!!!!!build os fail!!!!!!!!!!!!!!!')
        return False

    libtool_os_dst_lib = INSTALL_PATH + '/os'
    libtool_src_libs = [INSTALL_PATH + '/libcomm.a',
                        INSTALL_PATH + '/libmars-boost.a',
                        INSTALL_PATH + '/libxlog.a',
                        BUILD_OUT_PATH + '/zstd/libzstd.a']
    if not libtool_libs(libtool_src_libs, libtool_os_dst_lib):
        return False

    clean(BUILD_OUT_PATH)
    os.chdir(BUILD_OUT_PATH)
    ret = os.system(IOS_BUILD_SIMULATOR_CMD)
    os.chdir(SCRIPT_PATH)
    if ret != 0:
        print('!!!!!!!!!!!build simulator fail!!!!!!!!!!!!!!!')
        return False
    
    libtool_simulator_dst_lib = INSTALL_PATH + '/simulator'
    if not libtool_libs(libtool_src_libs, libtool_simulator_dst_lib):
        return False

    lipo_src_libs = []
    lipo_src_libs.append(libtool_os_dst_lib)
    lipo_src_libs.append(libtool_simulator_dst_lib)
    lipo_dst_lib = INSTALL_PATH + '/mars'

    if not lipo_libs(lipo_src_libs, lipo_dst_lib):
        return False

    dst_framework_path = INSTALL_PATH + '/mars.framework'
    make_static_framework(lipo_dst_lib, dst_framework_path, XLOG_COPY_HEADER_FILES, '../')

    print('==================Output========================')
    print(dst_framework_path)


def build_ios_xlog_xcframework(tag=''):
    gen_mars_revision_file('comm', tag)
    version = tag if tag else '1.3.1'

    device_framework_path = INSTALL_PATH + '/iphoneos/mars.framework'
    simulator_framework_path = INSTALL_PATH + '/iphonesimulator/mars.framework'

    if not _build_ios_xlog_framework(XLOG_DEVICE_BUILD_PATH, 'OS', device_framework_path, version):
        return False

    if not _build_ios_xlog_framework(XLOG_SIMULATOR_BUILD_PATH, 'SIMULATORARM64', simulator_framework_path, version):
        return False

    if not _create_xlog_xcframework(device_framework_path, simulator_framework_path):
        return False

    print('==================Output========================')
    print(XLOG_XCFRAMEWORK_OUT)
    print(XLOG_XCFRAMEWORK_ZIP)
    return True



def gen_ios_project():
    gen_mars_revision_file('comm')
    clean(BUILD_OUT_PATH)
    os.chdir(BUILD_OUT_PATH)

    ret = os.system(GEN_IOS_OS_PROJ)
    os.chdir(SCRIPT_PATH)
    if ret != 0:
        print('!!!!!!!!!!!gen fail!!!!!!!!!!!!!!!')
        return False


    print('==================Output========================')
    print('project file: %s/%s' %(SCRIPT_PATH, BUILD_OUT_PATH))
    
    return True

def main():
    while True:
        if len(sys.argv) >= 2:
            if sys.argv[1] == 'xlog':
                build_ios_xlog(sys.argv[2] if len(sys.argv) >= 3 else '')
            elif sys.argv[1] == 'xlog-xcframework':
                build_ios_xlog_xcframework(sys.argv[2] if len(sys.argv) >= 3 else '')
            else:
                build_ios(sys.argv[1])
            break
        else:
            num = input('Enter menu:\n1. Clean && build mars.\n2. Clean && build xlog.\n3. Gen iOS mars Project.\n4. Clean && build xlog xcframework.\n5. Exit\n')
            if num == '1':
                build_ios()
                break
            if num == '2':
                build_ios_xlog()
                break
            elif num == '3':
                gen_ios_project()
                break
            elif num == '4':
                build_ios_xlog_xcframework()
                break
            elif num == '5':
                break
            else:
                build_ios()
                break

if __name__ == '__main__':
    main()
