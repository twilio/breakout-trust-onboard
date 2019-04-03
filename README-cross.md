# Cross-compiling tips

## General
Setting a cross-compilation environment consists of two parts

  1. Setting the compilation toolchain
  2. Setting the target root file system

The fisrt step is the easier one: you need to download the toolchain for your system and set two variables for cmake.

  * `CMAKE_C_COMPILER`
  * `CMAKE_CXX_COMPILER`

The second one is more tedious and involves populating the root file system with all the dependencies. One shortcut here is to use your device's actual mounted root device and it's copy.

Once the root file system is populated you can give it to cmake as `CMAKE_FIND_ROOT_PATH`.

Cross-compilation for Linux devices can be made easier with such tools as [OpenEmbedded/Yocto](https://www.openembedded.org/wiki/Main_Page) or [Buildroot](https://buildroot.org/). This document covers manual cross-compilation process.

## Raspbian
You can get Raspberry Pi's cross-compilation toolchain from Github:

```
    git clone https://github.com/raspberrypi/tools.git raspberrypi-tools
```

Make sure that the correct compiler is in your `PATH`:

```
    export PATH=/path/to/raspberrypi-tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64
```

We have also prepared a CMake [toolchain file](device_support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake) which you can give as `-DCMAKE_TOOLCHAIN_FILE=...` to cmake when cross-compiling.

At the time of writing there was one Raspberry Pi / Raspbian device supported, that is Raspberry Pi with [Seeed LTE CAT 1 HAT](https://wiki.seeedstudio.com/LTE_Cat_1_Pi_HAT/).

### Building ToB library and tool to install on the device
The most convenient way to have the SDK on your Raspbian device is to build a Debian package. The package will contain

  * `TwilioTrustOnboard` static library and headers to build natively on Raspberry Pi
  * `trust_onboard_tool` binary to extract certificates and available private key from the Trust Onboard SIM.
  * Platform-specific services to get your device online
  * **TODO: ppp scripts**

This package is designed to have no dependencies, so you don't have to bother about the sysroot. Nevertheless, please make sure that `RPI_ROOT` is not set (`unset RPI_ROOT`) as having it set will confuse the compiler. To cross-compile, build as usual with `cmake` setting the toolchain to the one for Raspberry Pi. Also set `TOB_DEVICE` to one if the supported devices.

```
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake -DTOB_DEVICE=Seeed-LTE_Cat_1_Pi_HAT ..
    make
```

After that you can create a debian package to install on your device running `cpack` from the same directory.

### Building Trust Onboard library for cross-compilation Linux sysroot
Building the SDK for cross-compilation is almost the same as [building it for deploying on the device](Building ToB library and tool to install on the device) except

  1. Device support scripts are not installed
  2. The SDK is installed to target sysroot instead of '/'

```
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake -DCMAKE_INSTALL_PREFIX=/path/to/target/sysroot ..
    make
    make install
```
