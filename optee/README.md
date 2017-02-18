# OP-TEE with Cooperative Semantic Reconstruction (CSR)
## Contents
1. [Introduction](#1-introduction)
2. [License](#2-license)
3. [Platforms supported](#3-platforms-supported)
    3. [Development board for community user](#31-development-board-for-community-user)
4. [Get and build OP-TEE software](#4-get-and-build-op-tee-software)
    4. [Prerequisites](#41-prerequisites)
    4. [Basic setup](#42-basic-setup)
    4. [HiKey](#43-hikey)
6. [Cooperative Semantic Reconstruction (CSR)](#6-cooperative-semantic-reconstruction-csr)
7. [Load driver, tee-supplicant and run xtest](#7-load-driver-tee-supplicant-and-run-xtest)
8. [Coding standards](#8-coding-standards)
    8. [checkpatch](#81-checkpatch)

## 1. Introduction
The `optee_os git`, contains the source code for the TEE in Linux using the
ARM&reg; TrustZone&reg; technology. This component meets the GlobalPlatform
TEE System Architecture specification. It also provides the TEE Internal core API
v1.1 as defined by the GlobalPlatform TEE Standard for the development of
Trusted Applications. 

The Trusted OS is accessible from the Rich OS (Linux) using the
[GlobalPlatform TEE Client API Specification v1.0](http://www.globalplatform.org/specificationsdevice.asp),
which also is used to trigger secure execution of applications within the TEE.

This is a copy of the [main OP-TEE](https://github.com/OP-TEE/optee_os), modified to include the defense: Cooperative
Semantic Reconstruction, we proposed in our paper `BOOMERANG: Exploiting 
the Semantic Gap in Trusted Execution Environments`

Refer [License](#2-License) for the original license.

---
## 2. License
The software is distributed mostly under the
[BSD 2-Clause](http://opensource.org/licenses/BSD-2-Clause) open source
license, apart from some files in the `optee_os/lib/libutils` directory
which are distributed under the
[BSD 3-Clause](http://opensource.org/licenses/BSD-3-Clause) or public domain
licenses.

---
## 3. Platforms supported
Several platforms are supported. In order to manage slight differences
between platforms, a `PLATFORM_FLAVOR` flag has been introduced.
The `PLATFORM` and `PLATFORM_FLAVOR` flags define the whole configuration
for a chip the where the Trusted OS runs. Note that there is also a
composite form which makes it possible to append `PLATFORM_FLAVOR` directly,
by adding a dash in-between the names. The composite form is shown below
for the different boards. For more specific details about build flags etc,
please read the file [build_system.md](documentation/build_system.md).  
However, as explained in our paper, we have tested our changes only on Hikey. 
We currently support only Hikey.

<!-- Please keep this list sorted in alphabetic order -->
| Platform | Composite PLATFORM flag | Publicly available? |
|----------|-------------------------|---------------------|
| [HiKey Board (HiSilicon Kirin 620)](https://www.96boards.org/products/hikey)|`PLATFORM=hikey`| Yes |

### 3.1 Development board for community user
For community users, we suggest using [HiKey board](https://www.96boards.org/products/ce/hikey/)
as development board. It provides detailed documentation including chip
datasheet, board schematics, source code, binaries etc on the download link at
the website.

---
## 4. Get and build OP-TEE software
There are a couple of different build options depending on the target you are
going to use. If you just want to get the software and compile it, then you
should follow the instructions under the "Basic setup" below. In case you are
going to run for a certain hardware or FVP, QEMU for example, then please follow
the respective section found below instead, having that said, we are moving from
the shell script based setups to instead use
[repo](https://source.android.com/source/downloading.html), so for some targets
you will see that we are using repo ([section 5](#5-repo-manifests)) and for
others we are still using the shell script based setup
([section 4](#4-get-and-build-op-tee-software)), please see this transitions as
work in progress.

---
### 4.1 Prerequisites
We believe that you can use any Linux distribution to build OP-TEE, but as
maintainers of OP-TEE we are mainly using Ubuntu-based distributions and to be
able to build and run OP-TEE there are a few packages that needs to be installed
to start with. Therefore install the following packages regardless of what
target you will use in the end.
```
$ sudo apt-get install android-tools-adb android-tools-fastboot autoconf bison \
               cscope curl flex gdisk libc6:i386 libfdt-dev \
               libglib2.0-dev libpixman-1-dev libstdc++6:i386 \
               libz1:i386 netcat python-crypto python-serial \
               python-wand uuid-dev xdg-utils xz-utils zlib1g-dev \
               mtools
```

---

### 4.2 Get the toolchains
This is a one time thing you run only once after getting all the source code.
```
$ cd build
$ make toolchains
```

### 4.3 Installing on HiKey
After getting the source and toolchain, just run (from the `build` folder)
```
$ make all
```

After that connect the board and flash the binaries by running:
```
$ make flash
```

(more information about how to flash individual binaries could be found
[here](https://github.com/96boards/documentation/wiki/HiKeyUEFI#flash-binaries-to-emmc-))

The board is ready to be booted.

### 4.4 Compiler flags
To be able to see the full command when building you could build using
following flag:
```
$ make V=1
```

To enable debug builds use the following flag:
```
$ make DEBUG=1
```

OP-TEE supports a couple of different levels of debug prints for both TEE core
itself and for the Trusted Applications. The level ranges from 1 to 4, where
four is the most verbose. To set the level you use the following flag:
```
$ make CFG_TEE_CORE_LOG_LEVEL=4
```

##### Good to know
Just want to update secure side? Put the device in fastboot mode and
```
$ make arm-tf
$ make flash-fip

```

Just want to update OP-TEE client software? Put the device in fastboot mode and
```
$ make optee-client
$ make xtest
```
## 6. Cooperative Semantic Reconstruction (CSR)
On secure side refer [teemanager](https://github.com/ucsb-seclab/boomerang/blob/master/optee/optee_os/core/arch/arm/kernel/tee_ta_manager.c#L89), on non-secure side refer [rpc](https://github.com/ucsb-seclab/boomerang/blob/master/optee/linux/drivers/tee/optee/rpc.c#L389).
## 7. Load driver, tee-supplicant and run xtest
Since release v2.0.0 you don't have to load the kernel driver explicitly. In the
standard configuration it will be built into the kernel directly. To actually
run something on a device you however need to run tee-supplicant. This is the
same for all platforms, so when a device has booted, then run
```
$ tee-supplicant &
```
and OP-TEE is ready to be used.

In case you want to try run something that triggers both normal and secure side
code you could run xtest (the main test suite for OP-TEE), run
```
$ xtest
```
All the timings for each test will be displayed with prefix "BOOMERANG Time for", you can see all the timing related logs using following command:
```
$ xtest | grep BOOMERANG
```

---
## 8. Coding standards
In this project we are trying to adhere to the same coding convention as used in
the Linux kernel (see
[CodingStyle](https://www.kernel.org/doc/Documentation/CodingStyle)). We achieve this by running
[checkpatch](http://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/scripts/checkpatch.pl)
from Linux kernel. However there are a few exceptions that we had to make since
the code also follows GlobalPlatform standards. The exceptions are as follows:

- CamelCase for GlobalPlatform types are allowed.
- And we also exclude checking third party code that we might use in this
  project, such as LibTomCrypt, MPA, newlib (not in this particular git, but
  those are also part of the complete TEE solution). The reason for excluding
  and not fixing third party code is because we would probably deviate too much
  from upstream and therefore it would be hard to rebase against those projects
  later on (and we don't expect that it is easy to convince other software
  projects to change coding style).

### 8.1 checkpatch
Since checkpatch is licensed under the terms of GNU GPL License Version 2, we
cannot include this script directly into this project. Therefore we have
written the Makefile so you need to explicitly point to the script by exporting
an environment variable, namely CHECKPATCH. So, suppose that the source code for
the Linux kernel is at `$HOME/devel/linux`, then you have to export like follows:

    $ export CHECKPATCH=$HOME/devel/linux/scripts/checkpatch.pl
thereafter it should be possible to use one of the different checkpatch targets
in the [Makefile](Makefile). There are targets for checking all files, checking
against latest commit, against a certain base-commit etc. For the details, read
the [Makefile](Makefile).

