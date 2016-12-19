Quick and dirty initramfs RootFS build script by Linus Walleij

This builds a rootfs-<arch>.cpio that can be attached to the kernel
during linking and used as a RAM-only rootfs. Ain't it great.
It builds initramfs disks for a few select ARM systems plus Intel
i486 actually (because I happened to need an i486 ramfs).

Download and unpack dependencies directly into the dir where you
clones this script. (One day I may add wget downloading!)

- Main script is generate-cpio-rootfs.sh
  You need to read and understand this script.

- The etc/ directory contain some files and variants of files
  that will end up in /etc on the ramdisk

- You will need to download and install the referenced cross
  compilers, busybox and strace versions.

- Just because I am me, there are some other test utils I
  usually compile in. They are directly in the source tree!

During building new dirs appear. They have obvious names.
