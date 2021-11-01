#!/bin/bash
set -e
cd "$(dirname "$0")"

# CPUS should be passed in by the builder, but in case that not (e.g. locally),
# let's not slow things down too much.
if [ -z "${CPUS}" ]; then
  CPUS=$(nproc)
fi

# Make COREBOOT toolchain known
. ./edksetup.sh
rm -f "${CONF_PATH}/tools_def.txt"
. ./edksetup.sh
cat "$(dirname $(command -v i386-elf-gcc))/../share/edk2config/tools_def.txt" >> "${CONF_PATH}/tools_def.txt"

rm -rf Build/
if [ -n "${COREBOOT_BUILD_DIR}" ]; then
  ln -s "${COREBOOT_BUILD_DIR}" Build
fi

# Ensure tools are ready
make -C BaseTools/Source/C -j "${CPUS}"

# Make sure that whatever we do, we don't break upstream use cases
build -t COREBOOT -j "${CPUS}" -a X64 -p OvmfPkg/OvmfPkgX64.dsc
build -t COREBOOT -j "${CPUS}" -a IA32 -p OvmfPkg/OvmfPkgIa32.dsc
build -t COREBOOT -j "${CPUS}" -a IA32 -a X64 -p OvmfPkg/OvmfPkgIa32X64.dsc

# Test coreboot payload configs
build -t COREBOOT -j "${CPUS}" -a IA32 -a X64 -p UefiPayloadPkg/UefiPayloadPkg.dsc -b DEBUG   -D BOOTLOADER=COREBOOT -D PS2_KEYBOARD_ENABLE
build -t COREBOOT -j "${CPUS}" -a IA32 -a X64 -p UefiPayloadPkg/UefiPayloadPkg.dsc -b RELEASE -D BOOTLOADER=COREBOOT -D PS2_KEYBOARD_ENABLE
