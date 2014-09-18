#!/bin/bash

TARGET=$1
if [ "$TARGET" != "" ]; then
	echo "starting your build for $TARGET"
else
	echo ""
	echo "you need to define your device target!"
	echo "example: build_sammy.sh n7100"
	exit 1
fi

if [ "$TARGET" = "i9300" ] ; then
CUSTOM_PATH=i9300
MODE=DUAL
elif [ "$TARGET" = "i9100" ] ; then
CUSTOM_PATH=i9100
MODE=CM
else
CUSTOM_PATH=note
MODE=DUAL
fi


displayversion=Devil3.4_0.3.9

version=$displayversion-$TARGET-$MODE-$(date +%Y%m%d)

if [ -e boot.img ]; then
	rm boot.img
fi

if [ -e compile.log ]; then
	rm compile.log
fi

if [ -e ramdisk.cpio ]; then
	rm ramdisk.cpio
fi

if [ -e ramdisk.cpio.gz ]; then
	rm ramdisk.cpio.gz
fi

# Set Default Path
KERNEL_PATH=$PWD

# Set toolchain and root filesystem path
if [ "$(whoami)" == "dominik" ]; then
	#TOOLCHAIN_PATH="/home/dominik/android/android_4.2/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin"
	#TOOLCHAIN_PATH="/home/dominik/android/android_4.2/prebuilt/linux-x86/toolchain/android-toolchain-eabi-4.8-2013.07/bin"
	TOOLCHAIN_PATH="/home/dominik/android/android_4.2/prebuilt/linux-x86/toolchain/android-toolchain-eabi-4.8-2013.12/bin"
	#TOOLCHAIN_PATH="/home/dominik/android/android_4.2/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7.2/bin"
elif [ "$(whoami)" == "rollus" ]; then
	TOOLCHAIN_PATH="/home/rollus/android-toolchain-eabi/bin/"
fi
TOOLCHAIN="$TOOLCHAIN_PATH/arm-eabi-"
ROOTFS_PATH="$KERNEL_PATH/ramdisks/$TARGET-combo"
MODULESDIR="$KERNEL_PATH/ramdisks/modules"
MODULES="$KERNEL_PATH/ramdisks/modules/lib/modules"

defconfig=cyanogenmod_"$TARGET"_defconfig

export LOCALVERSION="-$displayversion"
export KERNELDIR=$KERNEL_PATH
export CROSS_COMPILE=$TOOLCHAIN
export ARCH=arm

export USE_SEC_FIPS_MODE=true

# Set ramdisk files permissions
cd $ROOTFS_PATH
ls $ROOTFS_PATH/roms/ | while read ramdisk; do
	cd $ROOTFS_PATH/roms/$ramdisk
	echo fixing permisions on $(pwd)
chmod 644 *.rc
chmod 750 init*
chmod 640 fstab*
chmod 644 default.prop
done
chmod 750 $ROOTFS_PATH/sbin/init*
chmod a+x $ROOTFS_PATH/sbin/*.sh
cd $KERNEL_PATH

if [ "$2" = "clean" ]; then
echo "Cleaning latest build"
make -j`grep 'processor' /proc/cpuinfo | wc -l` mrproper
fi
# Cleaning old kernel and modules
find -name '*.ko' -exec rm -rf {} \;
rm -rf $KERNEL_PATH/arch/arm/boot/zImage

# Making our .config
make $defconfig

make -j`grep 'processor' /proc/cpuinfo | wc -l` || exit -1
# Copying and stripping kernel modules
if [ "$TARGET" == "i9100" ] ; then
MODULES=releasetools/$CUSTOM_PATH/zip/system/lib/modules
fi

mkdir -p $MODULES
find -name '*.ko' -exec cp -av {} $MODULES \;
        "$TOOLCHAIN"strip --strip-unneeded $MODULES/*


# Copy Kernel Image
rm -f $KERNEL_PATH/releasetools/$CUSTOM_PATH/tar/$version.tar
rm -f $KERNEL_PATH/releasetools/$CUSTOM_PATH/zip/$version.zip
cp -f $KERNEL_PATH/arch/arm/boot/zImage .

if [ "$TARGET" != "i9100" ] ; then
# Create ramdisk.cpio archive
cd $MODULESDIR
find . | fakeroot cpio -o -H newc > $KERNEL_PATH/ramdisk.cpio 2>/dev/null
ls -lh $KERNEL_PATH/ramdisk.cpio
gzip -9 $KERNEL_PATH/ramdisk.cpio
cd $KERNEL_PATH

# Make boot.img
./mkbootimg --kernel zImage --ramdisk ramdisk.cpio.gz --board smdk4x12 --base 0x10000000 --pagesize 2048 --ramdiskaddr 0x11000000 -o $KERNEL_PATH/boot.img

# Copy boot.img
cp boot.img $KERNEL_PATH/releasetools/$CUSTOM_PATH/zip
cp boot.img $KERNEL_PATH/releasetools/$CUSTOM_PATH/tar
else
cp zImage $KERNEL_PATH/releasetools/$CUSTOM_PATH/zip
cp zImage $KERNEL_PATH/releasetools/$CUSTOM_PATH/tar
fi

# Creating flashable zip and tar
cd $KERNEL_PATH
cd releasetools/$CUSTOM_PATH/zip
zip -0 -r $version.zip *
mkdir -p $KERNEL_PATH/release
mv *.zip $KERNEL_PATH/release
cd ..

if [ "$TARGET" != "i9100" ] ; then
cd tar
tar cf $version.tar boot.img && ls -lh $version.tar
mv *.tar $KERNEL_PATH/release
fi

# Cleanup
cd $KERNEL_PATH
rm $KERNEL_PATH/releasetools/$CUSTOM_PATH/zip/boot.img
rm $KERNEL_PATH/releasetools/$CUSTOM_PATH/tar/boot.img
rm $KERNEL_PATH/zImage
