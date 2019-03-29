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

### Creating cross-compilation environment for Azure IoT Hub
#### Preparing sysroot
To cross-compile for Raspbian, you will need Raspbian sysroot. The most straightforward way to get one, is to [download Raspbian](https://www.raspberrypi.org/downloads/raspbian/). Unfortunately Azure SDK also requires extra dependencies to be installed. To get these, you can [flash it to an SD card](https://www.raspberrypi.org/documentation/installation/installing-images/) run on an internet-connected Raspberry Pi and download the dependencies as

```
    sudo apt install libssl1.0-dev uuid-dev libcurl4-openssl-dev
```

After that you will need to copy two directories from there, namely `usr` and `lib` to a location on your development machine. You can do it either by mounting the SD card to your computer

```
    mkdir ~/rpi-sysroot
    mount /dev/mmcblk2 /mnt/dev0
    cp -r /mnt/dev0/{usr,lib} ~/rpi-sysroot
    umount /mnt/dev
```

or remotely with `rsync`

```
    mkdir ~/rpi-sysroot
    rsync -rl --safe-links pi@<your Pi identifier>:/{usr,lib} ~/rpi-sysroot
```

Either way the sysroot you get will be suitable to build for any device with the same Raspbian version.

Finally set `RPI_ROOT` to point to your copy of the sysroot.

```
    export RPI_ROOT=~/rpi-sysroot
```

#### Building Azure IoT SDK
The easiest way to build with Azure IoT SDK is you have it installed to your local copy of Raspberry Pi sysroot and use it when building your applications. You don't need to get SDK files back to your devices as SDK libraries are linked statically.

First get the Azure IoT SDK to some location on your machine

```
    git clone --recursive https://github.com/Azure/azure-iot-sdk-c.git
```

**TODO: this should go before the release**
For our examples we use a branch of the SDK

```
    cd azure-iot-sdk-c
    git checkout hsm_custom_data
    git submodule update --init --recursive
```

Provided that the toolchain is in your `PATH` and you have [prepared the sysroot](#preparing-sysroot) and set `RPI_ROOT` variable, you can proceed to the build.

```
    cd azure-iot-sdk-c/build_all/linux

    ./build.sh \
       --toolchain-file /path/to/Breakout_Trust_Onboard_SDK/device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake \
       --provisioning \
       --no_uploadtoblob \
       --install-path-prefix ${RPI_ROOT}/usr

    cd ../../cmake/iotsdk_linux
    make install
```

#### Building Trust Onboard SDK
Building the SDK for cross-compilation is almost the same as [building it for deploying on the device](Building ToB library and tool to install on the device) except

  1. Device support scripts are not installed
  2. The SDK is installed to `RPI_ROOT` instead of '/'

```
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake -DCMAKE_INSTALL_PREFIX=${RPI_ROOT} ..
    make
    make install
```

#### Building your application
Now that both [Azure IoT SDK](#building-azure-iot-sdk) and [Trust Onboard SDK](#building-trust-onboard-sdk) are built and installed, everything is set to build Azure IoT applications.


We have provided an [example](cloud-support/azure-iot/sample) of such an application. You can safely have your application outside of Trust Onboard SDK directory tree, the only thing you need to set up is `RPI_ROOT`. Let's go and build the sample.

```
    # We assume RPI_ROOT is set appropriately

    cd cloud-support/azure-iot/sample
    mkdir build
    cmake -DCMAKE_TOOLCHAIN_FILE=../../../../device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake ..
    make

    # optionally you can make a deb file to install on the device
    cpack
```
