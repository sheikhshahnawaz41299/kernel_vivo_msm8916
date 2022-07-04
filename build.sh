#!/bin/bash
#
# Copyright © 2016, Varun Chitre "varun.chitre15" <varun.chitre15@gmail.com>
# Copyright © 2017, Ritesh Saxena <riteshsax007@gmail.com>
# Copyright © 2018, Mohd Faraz <mohd.faraz.abc@gmail.com>
# Copyright © 2022, Shahnawaz Sheikh <sheikhshahnawaz41299>
#
# Custom build script
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Please maintain this if you use this script or any part of it
#

KERNEL_DIR=$PWD
KERN_IMG=$KERNEL_DIR/out/arch/arm/boot/zImage
BUILD_START=$(date +"%s")
blue='\033[0;34m'
cyan='\033[0;36m'
green='\e[0;32m'
yellow='\033[0;33m'
red='\033[0;31m'
nocol='\033[0m'
purple='\e[0;35m'
white='\e[0;37m'
DEVICE="pd1510"
J="-j$(grep -c ^processor /proc/cpuinfo)"
rm -rf $KERNEL_DIR/out
mkdir $KERNEL_DIR/out

# Get Toolchain

TOOLCHAIN=$KERNEL_DIR/../toolchain32
DTBTOOL=$KERNEL_DIR/flash_zip/tools/dtbTool

function TC() {

if [[ -d ${TOOLCHAIN} ]]; then
	if [[ -d ${TOOLCHAIN}/.git ]]; then
			cd ${TOOLCHAIN}
			git fetch origin
                        git reset --hard origin/master
                        git clean -fxd > /dev/null 2>&1
                        cd ${KERNEL_DIR}
	else
		rm -rf ${TOOLCHAIN}
	fi
else
	git clone https://github.com/sheikhshahnawaz41299/android_prebuilts_gcc_linux-x86_arm_arm-eabi-4.9.git $TOOLCHAIN
fi
}

# Modify the following variable if you want to build
export CROSS_COMPILE=$TOOLCHAIN/bin/arm-eabi-
export ARCH=arm
export SUBARCH=arm
export KBUILD_BUILD_USER="Shahnawaz"
export KBUILD_BUILD_HOST="-"
J="-j$(grep -c ^processor /proc/cpuinfo)"
BUILD_DIR=$KERNEL_DIR/flash_zip
VERSION="X1"
DATE=$(date -u +%Y%m%d-%H%M)
ZIP_NAME=CM12-$DEVICE-$VERSION-$DATE

compile_kernel ()
{
echo -e "$cyan****************************************************"
echo "             Compiling kernel        "
echo -e "****************************************************"
echo -e "$nocol"
rm -f $KERN_IMG
make O=out msm8916-perf_defconfig
make O=out $J
echo -e "$cyan****************************************************"
echo "             Makeing dt.img                     "
echo -e "****************************************************"
$DTBTOOL -s 2048 -o $KERNEL_DIR/out/arch/dt.img -p $KERNEL_DIR/scripts/dtc/ $KERNEL_DIR/out/arch/arm/boot/dts/


if ! [ -a $KERN_IMG ];
then
echo -e "$red Kernel Compilation failed! Fix the errors! $nocol"
exit 1
fi


make_zip
}


make_zip ()
{
echo -e "$green****************************************************$nocol"
echo -e "$blue            Makeing Flashable Zip        $nocol"
echo -e "$green****************************************************$nocol"
if [[ $( ls ${KERNEL_DIR}/out/arch/arm/boot/zImage 2>/dev/null | wc -l ) != "0" ]]; then
	BUILD_RESULT_STRING="BUILD SUCCESSFUL"
	echo "Making Zip"
	rm $BUILD_DIR/*.zip
	cp $KERNEL_DIR/out/arch/arm/boot/zImage $BUILD_DIR/tools/zImage
	cp $KERNEL_DIR/out/arch/dt.img $BUILD_DIR/tools/dt.img
	mkdir -p $BUILD_DIR/system/lib/modules/
	find $KERNEL_DIR/out -name '*.ko' -type f -exec cp '{}' $BUILD_DIR/system/lib/modules/ \;
	mkdir -p $BUILD_DIR/system/lib/modules/pronto/
	mkdir -p $BUILD_DIR/system/etc/wifi/
	mkdir -p $BUILD_DIR/system/etc/firmware/wlan/prima/
	cp $KERNEL_DIR/drivers/staging/prima/firmware_bin/WCNSS_cfg.dat $BUILD_DIR/system/etc/firmware/wlan/prima/WCNSS_cfg.dat
	cp $KERNEL_DIR/drivers/staging/prima/firmware_bin/WCNSS_qcom_wlan_nv.bin $BUILD_DIR/system/etc/firmware/wlan/prima/WCNSS_qcom_wlan_nv.bin
	cp $KERNEL_DIR/drivers/staging/prima/firmware_bin/WCNSS_qcom_cfg.ini $BUILD_DIR/system/etc/wifi/WCNSS_qcom_cfg.ini
	mv $BUILD_DIR/system/lib/modules/wlan.ko $BUILD_DIR/system/lib/modules/pronto/pronto_wlan.ko
	cd $BUILD_DIR
	zip -r ${ZIP_NAME}.zip *
    echo -e "$yellow****************************************************$nocol"
    echo -e "$red                        Cleaning                     $nocol"
    echo -e "$yellow****************************************************$nocol"
	cd $KERNEL_DIR
    rm -rf $KERNEL_DIR/out
	rm $BUILD_DIR/tools/zImage
	rm $BUILD_DIR/tools/dt.img
	rm -rf $BUILD_DIR/system

else
    BUILD_RESULT_STRING="BUILD FAILED"
fi
}
case $1 in
clean)
make ARCH=arm64 O=out-j8 clean mrproper
;;
*)
TC
compile_kernel
;;
esac

BUILD_END=$(date +"%s")
DIFF=$(($BUILD_END - $BUILD_START))
if [[ "${BUILD_RESULT_STRING}" = "BUILD SUCCESSFUL" ]]; then
echo -e "$cyan****************************************************************************************$nocol"
echo -e "$cyan*$nocol${red} ${BUILD_RESULT_STRING}$nocol"
echo -e "$cyan*$nocol$yellow Build completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds.$nocol"
echo -e "$cyan*$nocol${green} ZIP LOCATION: ${BUILD_DIR}/${ZIP_NAME}.zip$nocol"
echo -e "$cyan*$nocol${green} SIZE: $( du -h ${BUILD_DIR}/${ZIP_NAME}.zip | awk '{print $1}' )$nocol"
echo -e "$cyan****************************************************************************************$nocol"
fi
