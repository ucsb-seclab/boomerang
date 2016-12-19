#!/bin/bash
#
# Usage: generate_cpio_rootfs.sh <platform_name>
#        generate_cpio_rootfs.sh <platform_name> clean
#        generate_cpio_rootfs.sh <platform_name> nocpio
#
# All credits to Linus Walleij that wrote this initially.

# exit as soon as an error is encoutered
set -e

echo "Generator for a simple initramfs root filesystem for some ARM targets"
CURDIR="$(dirname "$(readlink -f "$0")")"
STAGEDIR=${CURDIR}/stage
BUILDDIR=${CURDIR}/build
OUTFILE=${CURDIR}/rootfs-$1.cpio
BUSYBOXDIR=${CURDIR}/../busybox

if [ "$2" = "clean" -o "$2" = "distclean" ]; then
    echo "Cleaning"
    rm -rf ${STAGEDIR} ${BUILDDIR}
    rm -f ${OUTFILE} ${CURDIR}/etc/hostname ${CURDIR}/etc/inittab
    rm -f ${CURDIR}/filelist-tmp.txt ${CURDIR}/filelist-final.txt
    exit 0
fi

STRACEVER=strace-4.7
STRACE=${CURDIR}/${STRACEVER}

# Helper function to copy one level of files and then one level
# of links from a directory to another directory.
function clone_dir()
{
    SRCDIR=$1
    DSTDIR=$2
    FILES=`find ${SRCDIR} -maxdepth 1 -type f`
    for file in ${FILES} ; do
        BASE=`basename $file`
        cp $file ${DSTDIR}/${BASE}
        # ${STRIP} -s ${DSTDIR}/${BASE}
    done;

    # Clone links from the toolchain binary library dir
    LINKS=`find ${SRCDIR} -maxdepth 1 -type l`
    cd ${DSTDIR}
    for file in ${LINKS} ; do
        BASE=`basename $file`
        TARGET=`readlink $file`
        ln -sf ${TARGET} ${BASE}
    done;
    cd ${CURDIR}
}

CC_PREFIX=`${CROSS_COMPILE}gcc -print-multiarch`
case "${CC_PREFIX}" in
aarch64-linux-gnu)
    LIBCBASE=$(dirname $(${CROSS_COMPILE}gcc -print-file-name=ld-linux-aarch64.so.1))/..
    export ARCH=arm64
    ;;
arm-linux-gnueabihf)
    LIBCBASE=$(dirname $(${CROSS_COMPILE}gcc -print-file-name=ld-linux-armhf.so.3))/..
    export ARCH=arm
    ;;
*)
    echo "Unsupported arch"
    exit 1
    ;;
esac

case $1 in
    "vexpress")
        CFLAGS=${CFLAGS-"-Wno-strict-aliasing -Wno-unused-result -marm -mabi=aapcs-linux -mthumb -mthumb-interwork -mcpu=cortex-a15"}
        cp etc/inittab-vexpress etc/inittab
        echo "Vexpress" > etc/hostname
        ;;

    "fvp")
        echo "Building FVP root filesystem"
        cp etc/inittab-vexpress etc/inittab
        echo "FVP" > etc/hostname
        ;;

    "hikey")
        echo "Building HiKey root filesystem"
        cp etc/inittab-hikey etc/inittab
        echo "HiKey" > etc/hostname
        ;;

    "mt8173-evb")
        cp etc/inittab-mt8173 etc/inittab
        echo "MT8173_EVB" > etc/hostname
        ;;

    *)
        echo "Usage: $0 [versatile|vexpress|fvp|hikey|mt8173-evb]"
        exit 1
        ;;
esac

# Define more tools
STRIP=${CROSS_COMPILE}strip

echo "OUTFILE = ${OUTFILE}"

echo "Check prerequisites..."
echo -n "Check crosscompiler ... "
${CROSS_COMPILE}gcc --version > /dev/null ; if [ ! $? -eq 0 ] ; then
    echo "ERROR: cross-compiler ${CROSS_COMPILE}gcc cannot be run!"
    echo "ABORTING."
    exit 1
else
    echo "OK"
fi

echo -n "Check # of CPUs ..."
_NPROCESSORS_ONLN=`getconf _NPROCESSORS_ONLN`
echo $_NPROCESSORS_ONLN

if [ "$2" != "nocpio" ]; then
    echo -n "gen_init_cpio ... "
    which gen_init_cpio > /dev/null ; if [ ! $? -eq 0 ] ; then
        echo "ERROR: gen_init_cpio not in PATH=$PATH!"
        echo "Copy this binary from the Linux build tree."
        echo "Or set your PATH into the Linux kernel tree, I don't care..."
        echo "ABORTING."
        exit 1
    else
        echo "OK"
    fi
fi

# Clone the busybox git if we don't have it...
if [ ! -d ${BUSYBOXDIR} ] ; then
    echo "It appears we're missing a busybox git"
    echo "Failed. ABORTING."
    exit 1
fi

# Copy the template of static files to be used
cp filelist.txt filelist-tmp.txt

# Prep dirs
mkdir -p ${STAGEDIR}
mkdir -p ${STAGEDIR}/lib
mkdir -p ${STAGEDIR}/sbin
mkdir -p ${BUILDDIR}

# For using the git version
cd ${BUSYBOXDIR}
if [ ! -e ${BUILDDIR}/.config ]; then
  make O=${BUILDDIR} defconfig
  echo "Configuring cross compiler etc..."
  # Comment in this line to create a statically linked busybox
  #sed -i "s/^#.*CONFIG_STATIC.*/CONFIG_STATIC=y/" ${BUILDDIR}/.config
  sed -i -e "s|CONFIG_CROSS_COMPILER_PREFIX=\"\"|CONFIG_CROSS_COMPILER_PREFIX=\"${CROSS_COMPILE}\"|g" ${BUILDDIR}/.config
  sed -i -e "s/CONFIG_EXTRA_CFLAGS=\"\"/CONFIG_EXTRA_CFLAGS=\"${CFLAGS}\"/g" ${BUILDDIR}/.config
  sed -i -e "s/CONFIG_PREFIX=\".*\"/CONFIG_PREFIX=\"..\/stage\"/g" ${BUILDDIR}/.config
  # Turn off "eject" command, we don't have a CDROM
  sed -i -e "s/CONFIG_EJECT=y/\# CONFIG_EJECT is not set/g" ${BUILDDIR}/.config
  sed -i -e "s/CONFIG_FEATURE_EJECT_SCSI=y/\# CONFIG_FEATURE_EJECT_SCSI is not set/g" ${BUILDDIR}/.config
  #make O=${BUILDDIR} menuconfig
fi
make -j${_NPROCESSORS_ONLN} O=${BUILDDIR}
make O=${BUILDDIR} install
cd ${CURDIR}

# First the flat library where arch-independent stuff will
# end up
clone_dir ${LIBCBASE}/lib ${STAGEDIR}/lib

# The C library may be in a per-arch subdir (multiarch)
# OR it may not...
if [ -d ${LIBCBASE}/lib/${CC_PREFIX} ] ; then
    mkdir -p ${STAGEDIR}/lib/${CC_PREFIX}
    echo "dir /lib/${CC_PREFIX} 755 0 0" >> filelist-tmp.txt
    clone_dir ${LIBCBASE}/lib/${CC_PREFIX} ${STAGEDIR}/lib/${CC_PREFIX}
fi

# Add files by searching stage directory
BINFILES=`find ${STAGEDIR}/bin -maxdepth 1 -type f`
for file in ${BINFILES} ; do
    BASE=`basename $file`
    echo "file /bin/${BASE} $file 755 0 0" >> filelist-tmp.txt
done;

SBINFILES=`find ${STAGEDIR}/sbin -maxdepth 1 -type f`
for file in ${SBINFILES} ; do
    BASE=`basename $file`
    echo "file /sbin/${BASE} $file 755 0 0" >> filelist-tmp.txt
done;

LIBFILES=`find ${STAGEDIR}/lib -maxdepth 1 -type f -a -not -name "*.a"`
for file in ${LIBFILES} ; do
    BASE=`basename $file`
    echo "file /lib/${BASE} $file 755 0 0" >> filelist-tmp.txt
done;

LIBLINKS=`find ${STAGEDIR}/lib -maxdepth 1 -type l`
for file in ${LIBLINKS} ; do
    BASE=`basename $file`
    TARGET=`readlink $file`
    echo "slink /lib/${BASE} ${TARGET} 755 0 0" >> filelist-tmp.txt
done;

# Add multiarch libarary dir
if [ -d ${STAGEDIR}/lib/${CC_PREFIX} ] ; then
    echo "dir /lib/${CC_PREFIX} 755 0 0" >> filelist-tmp.txt
    CLIBFILES=`find ${STAGEDIR}/lib/${CC_PREFIX} -maxdepth 1 -type f`
    for file in ${CLIBFILES} ; do
        BASE=`basename $file`
        echo "file /lib/${CC_PREFIX}/${BASE} $file 755 0 0" >> filelist-tmp.txt
    done;
    CLIBLINKS=`find ${STAGEDIR}/lib/${CC_PREFIX} -maxdepth 1 -type l`
    for file in ${CLIBLINKS} ; do
        BASE=`basename $file`
        TARGET=`readlink $file`
        echo "slink /lib/${CC_PREFIX}/${BASE} ${TARGET} 755 0 0" >> filelist-tmp.txt
    done;
fi

# Add links by searching stage directory
LINKSBIN=`find ${STAGEDIR}/bin -maxdepth 1 -type l`
for file in ${LINKSBIN} ; do
    BASE=`basename $file`
    TARGET=`readlink $file`
    echo "slink /bin/${BASE} ${TARGET} 755 0 0" >> filelist-tmp.txt
done;

LINKSSBIN=`find ${STAGEDIR}/sbin -maxdepth 1 -type l`
for file in ${LINKSSBIN} ; do
    BASE=`basename $file`
    TARGET=`readlink $file`
    echo "slink /sbin/${BASE} ${TARGET} 755 0 0" >> filelist-tmp.txt
done;

LINKSUSRBIN=`find ${STAGEDIR}/usr/bin -maxdepth 1 -type l`
for file in ${LINKSUSRBIN} ; do
    BASE=`basename $file`
    TARGET=`readlink $file`
    echo "slink /usr/bin/${BASE} ${TARGET} 755 0 0" >> filelist-tmp.txt
done;

LINKSUSRSBIN=`find ${STAGEDIR}/usr/sbin -maxdepth 1 -type l`
for file in ${LINKSUSRSBIN} ; do
    BASE=`basename $file`
    TARGET=`readlink $file`
    echo "slink /sbin/${BASE} ${TARGET} 755 0 0" >> filelist-tmp.txt
done;

# Extra stuff per platform
case $1 in
    "vexpress")
        ;;
    "fvp")
        ;;
    "hikey")
        ;;
    "mt8173-evb")
        ;;
    *)
        echo "Forgot to update special per-platform rules."
        exit 1
        ;;
esac

diff filelist-final.txt filelist-tmp.txt >/dev/null 2>&1 || mv filelist-tmp.txt filelist-final.txt

if [ "$2" != "nocpio" ]; then
    gen_init_cpio filelist-final.txt > ${OUTFILE}
    if [ -f ${OUTFILE} ] ; then
        echo "New rootfs ready in ${OUTFILE}"
    fi
fi

