#!/bin/bash

ROOT=$1
shift
if [ -z ${ROOT} ]; then
	echo "ROOT hasn't been set, please make sure it is set either in a"
	echo "Makefile or as an enviroment variable."
	exit 1
fi

export PATH=${ROOT}/toolchains/aarch32/bin:$PATH

export TA_DEV_KIT_DIR=${ROOT}/optee_os/out/arm-plat-vexpress/export-ta_arm32
export TEEC_EXPORT=${ROOT}/optee_client/out/export
export PLATFORM=vexpress
export PLATFORM_FLAVOR=qemu_virt

TA_CROSS_COMPILE=arm-linux-gnueabihf-
HOST_CROSS_COMPILE=arm-linux-gnueabihf-

# Build the host application
cd ${ROOT}/trasher
make CROSS_COMPILE=$HOST_CROSS_COMPILE $@

# Build the Trusted Application
cd ${ROOT}/trasher/ta
make O=${ROOT}/trasher/ta/out \
		CROSS_COMPILE=$TA_CROSS_COMPILE
                $@
