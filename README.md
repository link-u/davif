# davif

| | Status |
|--|---|
| Linux   | [![Build on Linux](https://github.com/link-u/davif/workflows/Build%20on%20Linux/badge.svg)](https://github.com/link-u/davif/actions?query=workflow%3A%22Build+on+Linux%22) |
| Linux(.deb) | [![Build debian packages](https://github.com/link-u/davif/workflows/Build%20debian%20packages/badge.svg)](https://github.com/link-u/davif/actions?query=workflow%3A%22Build+debian+packages%22) |
| macOS   | [![Build on macOS](https://github.com/link-u/davif/workflows/Build%20on%20macOS/badge.svg)](https://github.com/link-u/davif/actions?query=workflow%3A%22Build+on+macOS%22) |
| Windows | [![Build on Windows](https://github.com/link-u/davif/workflows/Build%20on%20Windows/badge.svg)](https://github.com/link-u/davif/actions?query=workflow%3A%22Build+on+Windows%22) |

## Description (en)

avif decoder, using [dav1d](https://code.videolan.org/videolan/dav1d) directly.

[avif (AV1 Image File Format)](https://aomediacodec.github.io/av1-avif/) is a still picture format uses a keyframe of [AV1](https://aomediacodec.github.io/av1-spec/av1-spec.pdf).

## Description (ja)

[AVIF(AV1 Image File Format)]((https://aomediacodec.github.io/av1-avif/))は、動画フォーマットである[AV1](https://aomediacodec.github.io/av1-spec/av1-spec.pdf)のキーフレームを流用して圧縮する静止画フォーマットです。

davifは、ラッパーを介さず[dav1d](https://code.videolan.org/videolan/dav1d)を直接叩くavifのデコード・コマンドです。

## how to build

```bash
# pre-requirements
# If your system cmake is lower than 3.13, please install latest version:
# https://apt.kitware.com/

# cloning this repository with dependencies.
git clone --recurse-submodules --recursive git@github.com:link-u/davif.git
cd davif

# System gcc is 8.0 or higher:
cmake ..

# If not, please install gcc-8 (or higher) and tell them to CMake.
CXX=g++-8 CC=gcc-8 cmake ..

# build davif binary.
make davif

# decode an avif image.
./davif -i input.avif -o output.png
```

## usage

```bash
% davif
[2020/03/18 15:40:21 INFO ] davif
[2020/03/18 15:40:21 DEBUG]  - dav1d ver: 0.6.0-13-gfe52bff
SYNOPSIS
        davif -i <input.avif> -o <output.png> [--extract-alpha <output-alpha.png>] [--extract-depth
              <output-depth.png>] [--threads <Num of threads to use>]
```

Example avif files are available in [AOMediaCodec/av1-avif](https://github.com/AOMediaCodec/av1-avif/tree/master/testFiles) or [link-u/avif-sample-images](https://github.com/link-u/avif-sample-images).

(Currently, detailed documentation is only in [Japanese](./doc/ja_JP/README.md))

## TODO

 - Add more and more command-line flags.

# Related repositories

 - [link-u/cavif](https://github.com/link-u/cavif) - avif encoder, using libaom directly.
 - [link-u/libavif-container](https://github.com/link-u/libavif-container) - a library to parse avif container.
 - [link-u/avif-sample-images](https://github.com/link-u/avif-sample-images) - sample images from us.
