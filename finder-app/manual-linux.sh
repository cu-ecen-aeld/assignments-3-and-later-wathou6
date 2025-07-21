#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	# git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
	git clone -v --depth 1 --single-branch --branch ${KERNEL_VERSION} ${KERNEL_REPO}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout tags/${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "Building kernel"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) all 
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) dtbs


fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
echo "Creating necessary base directories in ${OUTDIR}/rootfs"
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs

echo "Creating base directories in rootfs"
mkdir -p bin dev etc home lib proc sbin sys tmp usr/bin usr/lib usr/sbin var lib64

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "Configuring busybox"
    make distclean
    sed -i 's/default BASH_IS_NONE/default BASH_IS_ASH/' "shell/Config.src"
    make defconfig
    # Comment the line containing CONFIG_BASH_IS_NONE
    #sed -i 's/^\([^#]*\)CONFIG_BASH_IS_NONE.*/# \1CONFIG_BASH_IS_NONE/' ".config"
    # Set CONFIG_BASH_IS_ASH=y
    #sed -i '/CONFIG_BASH_IS_ASH/c\CONFIG_BASH_IS_ASH=y' ".config"
else
    cd busybox
fi

# TODO: Make and install busybox
echo "Building busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)
echo "Installing busybox"
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "Adding library dependencies to rootfs"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -a ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp -a ${SYSROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64
cp -a ${SYSROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
cp -a ${SYSROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64

# TODO: Make device nodes
echo "Creating device nodes"
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/zero c 1 5
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/tty c 5 0

# TODO: Clean and build the writer utility
echo "Building the writer utility"
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "Copying finder related scripts and executables to the /home directory on the target rootfs"
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
mkdir -p ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf/
cp ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/conf/
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/

# TODO: Chown the root directory
echo "Changing ownership of the root directory to root:root"
sudo chown -R root:root ${OUTDIR}/rootfs


# TODO: Create initramfs.cpio.gz
echo "Creating initramfs.cpio.gz"
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root | gzip > ${OUTDIR}/initramfs.cpio.gz

echo "Created initramfs.cpio.gz in ${OUTDIR}"
