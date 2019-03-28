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

To cross-compile, build as usual with `cmake` setting the compiler to the one for Raspberry Pi. Also set `TOB_DEVICE` to one if the supported devices (only `Seeed-LTE_Cat_1_Pi_HAT` at the time of writing).

```
    mkdir build
    cd build
    cmake -DCMAKE_C_COMPILER=/path/to/raspberrypi-tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -DCMAKE_CXX_COMPILER=/path/to/raspberrypi-tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ -DTOB_DEVICE=Seeed-LTE_Cat_1_Pi_HAT ..
    make
```

After that you can create a debian package to install on your device running `cpack` from the same directory.

TODO: building Azure sample requires root FS setup and is not covered yet
