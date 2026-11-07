#!/usr/bin/env python3
"""Build libarchive with actual static zlib references for the Windows SDK."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
from windows_deps import download

URL = 'https://github.com/libarchive/libarchive/releases/download/v3.8.11/libarchive-3.8.11.tar.xz'
SHA256 = 'a3dc0872b1b39da34dc18ef1b944f0f11ffd37425d70ae04be679279f1cf43d2'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--prefix', default='x86_64-w64-mingw32')
    args = parser.parse_args()
    sdk = args.sdk.resolve()
    if not shutil.which('cmake'):
        parser.error('cmake is required for static Windows dependencies')
    work = sdk.parent / 'static-work'
    work.mkdir(parents=True, exist_ok=True)
    archive = work / 'libarchive-3.8.11.tar.xz'
    download(URL, archive, SHA256)
    source = work / 'libarchive-3.8.11'
    if not source.exists():
        subprocess.run(['bsdtar', '-xf', str(archive), '-C', str(work)], check=True)
    build = work / 'build-libarchive'
    env = dict(os.environ, PKG_CONFIG_PATH='', PKG_CONFIG_LIBDIR=str(sdk / 'lib/pkgconfig'),
               PKG_CONFIG_SYSROOT_DIR=str(sdk))
    flags = f'-mcrtdll=msvcrt-os -B{sdk}/lib/ -DZLIB_STATIC -UZLIB_DLL -DLZMA_API_STATIC'
    command = ['cmake', '-S', str(source), '-B', str(build), '-G', 'Unix Makefiles',
               '-DCMAKE_SYSTEM_NAME=Windows', '-DCMAKE_BUILD_TYPE=Release',
               '-DCMAKE_C_COMPILER=' + args.prefix + '-gcc',
               '-DCMAKE_C_FLAGS=' + flags, '-DCMAKE_FIND_ROOT_PATH=' + str(sdk),
               '-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER',
               '-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY',
               '-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY',
               '-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY',
               '-DBUILD_SHARED_LIBS=OFF', '-DENABLE_TEST=OFF', '-DENABLE_TAR=OFF',
               '-DENABLE_CPIO=OFF', '-DENABLE_CAT=OFF', '-DENABLE_UNZIP=OFF',
               '-DOPENSSL_USE_STATIC_LIBS=TRUE', '-DZLIB_USE_STATIC_LIBS=TRUE',
               '-DZLIB_LIBRARY=' + str(sdk / 'lib/libz.a'),
               '-DZLIB_INCLUDE_DIR=' + str(sdk / 'include')]
    subprocess.run(command, check=True, env=env)
    subprocess.run(['cmake', '--build', str(build), '--target', 'archive_static',
                    '--parallel', '2'], check=True, env=env)
    destination = sdk / 'static/lib'
    destination.mkdir(parents=True, exist_ok=True)
    shutil.copy2(build / 'libarchive/libarchive.a', destination / 'libarchive.a')


if __name__ == '__main__':
    main()
