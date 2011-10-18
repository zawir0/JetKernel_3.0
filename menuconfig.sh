#/bin/bash
ANDROIDSRC=../android
CCOMPILER=$ANDROIDSRC/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
make ARCH=arm INITRAMFS_SOURCE_PATH=. CROSS_COMPILE=$CCOMPILER menuconfig

