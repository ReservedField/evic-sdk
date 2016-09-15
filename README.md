eVic SDK is a software development kit for writing APROMs for the Joyetech eVic VTC Mini.

Setting up the environment
--------------------------

To use evic-sdk, you need a working arm-none-eabi GCC toolchain,
binutils and libc. On Linux, most distros have precompiled packages
in their repos. For example, on Fedora, install the following
packages:
```
arm-none-eabi-gcc
arm-none-eabi-newlib
```
On Ubuntu, the following should be enough:
```
gcc-arm-none-eabi
libnewlib-arm-none-eabi
```
On OSX you can use [brew](http://brew.sh/):
```
brew tap mpaw/arm-none-eabi
brew update
brew install gcc-arm-none-eabi
```

On Windows, first install the [precompiled ARM toolchain](https://launchpad.net/gcc-arm-embedded).
Choose an installation path without spaces to avoid problems with the build process.
If you already have a working Windows `make` and `git` along with standard GNU utilities you can go
ahead, but make sure those binaries are in your `PATH`. Otherwise, install [Cygwin](https://www.cygwin.com/)
and add the following packages on top of the base install:
```
make
git
```

On any OS, you also need a working [python-evic](https://github.com/Ban3/python-evic) install.
If you experience problems with hidapi, you can flash using the official updater. The conversion
utilies needed by the SDK will work anyway.

#### Installing python-evic on Cygwin

On Cygwin, hidapi (needed by python-evic) won't build as-is. There are various issues
(Cygwin not recognized as a target, DLL naming conflict, HID open permissions).
You have two options: you can install python-evic just for the conversion part which doesn't
require hidapi and use the official updater for flashing, or patch hidapi to make it work
on Cygwin. In either case, you need to install python-evic in the first place:

1. Install the following Cygwin packages:

   ```
   python3
   python3-setuptools
   ```

2. Download and install python-evic:

   ```
   git clone https://github.com/Ban3/python-evic
   cd python-evic
   python3 setup.py install
   ```

If you want to patch hidapi to get flashing functionality from python-evic, follow
those instructions:

1. Install the following Cygwin packages:

   ```
   binutils
   gcc-core
   gcc-g++
   libhidapi0
   libhidapi-devel
   libusb1.0
   libusb1.0-devel
   wget
   patch
   ```

2. Download, patch and install hidapi:

   ```
   wget https://pypi.python.org/packages/source/h/hidapi/hidapi-0.7.99.post12.tar.gz
   wget http://pastebin.com/raw/16E7UdNF && echo >> 16E7UdNF
   tar -zxvf hidapi-0.7.99.post12.tar.gz
   patch -s -p0 < 16E7UdNF
   cd hidapi-0.7.99.post12
   python3 setup.py install
   ```

Installation
------------

1. Clone this repository:
   ```
   git clone https://github.com/ReservedField/evic-sdk.git
   cd evic-sdk
   ```

2. Download the latest [M451 series SDK](http://www.nuvoton.com/hq/support/tool-and-software/software)
   from Nuvoton and copy the `Library` folder inside `evic-sdk/nuvoton-sdk`, as to have
   `evic-sdk/nuvoton-sdk/Library`.

3. Point the `EVICSDK` environment variable to the `evic-sdk` folder. For example, if you're using
   bash and are in the SDK directory:
   ```
   echo "export EVICSDK=$(pwd)" >> $HOME/.bashrc
   ```
   Make sure to restart your terminal to ensure variables are set before building.

4. Build the SDK:
   ```
   make
   ```

Building your first APROM
--------------------------

The `helloworld` example should be the first thing you try compiling and flashing,
to check that everything is working correctly. Build it like:
```
cd example/helloworld
make
```
If the build succeeds, you should now have a `bin/rel/DEVICE/helloworld.bin` file ready to flash.
This file is encrypted and compatible with the official updater.

Flashing
--------

You can flash the output binary using the official updater. For development,
using `python-evic` is quicker and simpler:
```
evic-usb upload bin/rel/DEVICE/helloworld.bin
```
If everything went well you should see the "Hello, World." message.

This APROM doesn't include USB updating, so you need to reboot to LDROM to flash something
else. To do it, remove the battery and disconnect the USB cable. Then, holding the right button,
connect the USB cable. Now you can let the button go and flash away. You can also insert the
battery while the button is pressed, then let it go and connect the cable. I find powering
over USB is more convenient (as long as the APROM doesn't require significant power, i.e.
it doesn't fire the atomizer). Similiarly, holding the left button during powerup will force
the system to boot from APROM.

Unless you're messing with the LDROM, this is practically unbrickable - you can always boot to
LDROM and restore. Actually, APROM update is always done from LDROM: the official APROM reboots
to LDROM and flashing happens from there.

The build system
----------------

The build system, used both by the SDK and APROMs, works around *specifiers*. A specifier is a
string in the form `device-flavor`, where `device` is a supported device and `flavor` is a build
flavor.

**Supported devices:**

- `evic`: Joyetech eVic VTC Mini
- `all`: all of the above

**Build flavors:**

- `dbg`: debug build
- `rel`: release build
- `all`: all of the above

See below for the differences between debug and release builds. The `all-all` specifier is also
aliased to `all`. There are two classes of make targets:

- `specifier`: performs a build for the specified device and flavor
- `clean-specifier`: cleans the build for the specified device and flavor

Some examples (if more than one command is given, they are alternatives):
```
# Build everything
make all
make all-all
# Clean everything
make clean-all
make clean-all-all
# Build for eVic VTC Mini, debug flavor
make evic-dbg
# Build for eVic VTC Mini, all flavors
make evic-all
# Clean for eVic VTC Mini, release flavor
make clean-evic-rel
# Build for all devices, release flavor
make all-rel
# Clean for all devices, debug flavor
make clean-all-dbg
# Mix & match
make all-rel evic-dbg
```

#### Default targets

While the specifier system lets you specify exactly what you want, it can feel too verbose for
day-to-day development. For this reason, the build system accepts two environment variables:

- `EVICSDK_MAKE_DEFAULT_DEVICE`: default device(s)
- `EVICSDK_MAKE_DEFAULT_FLAVOR`: default flavor(s)

Those can be set to a single device/flavor or to a list of them (for example, default flavors
`dbg rel` and `all` are the same). You can omit the device, the flavor or both from any specifier
and they will be filled in from those variables. Unset or empty variables default to `all`. The
empty target is also aliased to `def`. Some examples:
```
# Build for default device(s) and flavor(s)
make
make def
# Clean for default device(s) and flavor(s)
make clean
# Build for default device(s), debug flavor
make dbg
# Build for eVic VTC Mini, default flavor(s)
make evic
# Clean for default devices(s), release flavor
make clean-rel
```
If you're writing scripts around the build system don't assume the user's defaults and use the
full specifiers (unless you actually want to use the user's defaults).

#### Object files

Objects files for a specific device and flavor live in the `obj/FLAVOR/DEVICE` directory. The
source tree is replicated in there, to avoid conflicts between files with the same name. If
you wanted to compile the file `src/foo/bar.c` for an eVic VTC Mini, debug flavor, you could
issue `make obj/dbg/evic/src/foo/bar.o`.

#### SDK builds

SDK output files live in the `lib/FLAVOR/DEVICE` directory. Debug builds enable debug info
generation from the compiler and a fault handler that will display crash and register info, which
can be decoded using the script in `tools/fault-decode`.

You can generate Doxygen documentation (`doc` directory) for the SDK using `make docs`. To clean it
use `make clean-docs`.

#### APROM builds

APROM output files live in the `bin/FLAVOR/DEVICE` directory. The standard Makefile for APROMs
follows this scheme:
```
TARGET = helloworld
OBJS = main.o
include $(EVICSDK)/make/Base.mk
```
`TARGET` is the base name for outputs. `OBJS` is a list of objects to be built. Notice that, even
though objects live in `obj/FLAVOR/DEVICE`, the object list is written as if they were in the same
directory as the sources. For example, `src/foo/bar.c` would appear as `src/foo/bar.o` in the list.
This is done so that you can let the build system worry about devices and flavors (and to preserve
compatibility with Makefiles written for the old build system). Since the source tree is replicated
for the objects, **always use relative paths without `..` components** (you can't have sources at
a higher level than the Makefile). Finally, `make/Base.mk` has to be included (**after** setting
those variables). See the top of `make/Base.mk` for more variables you can use.

The main output is `bin/FLAVOR/DEVICE/TARGET.bin`, which is an encrypted binary compatible with the
official updater. Debug builds enable debug info generation from the compiler and generate some
additional outputs:

- `bin/dbg/DEVICE/TARGET.elf`: ELF output from compilation
- `bin/dbg/DEVICE/TARGET_dec.bin`: unencrypted binary
- `bin/dbg/DEVICE/TARGET.map`: linker map

Also, they are built against the debug SDK, so the fault handler is enabled. All the outputs except
the linker map are targets (so you could `make bin/dbg/DEVICE/TARGET.elf`, for example).

Note that APROMs for a certain device and flavor combination are built against the SDK for that
same device and flavor, so you'll need to have that SDK built for it to succeed. If you see linker
errors about missing `evicsdk-crt0.o`, `-levicsdk` or `-lnuvosdk`, or you get
`No SDK found for DEVICE-FLAVOR`, chances are you don't have the needed SDK built.

#### Parallel make

Parallel make (`-j` option) is supported and works as you would expect. However, problems can arise
when you mix build and clean targets. For example, a default rebuild can be performed with
`make clean && make`. For single-threaded make this could be shortened to `make clean def`, because
targets will be made one at a time from left to right. With parallel make you will need to use the
full form because the two targets would likely be made concurrently, breaking the build. Of course,
mixing multiple build targets or multiple clean targets poses no issues.

#### Clang support

Clang is supported: just build with `CC=clang make`. At the moment, the support still depends on
arm-none-eabi GCC for some standard headers and libraries. It will also generate a fair amount of
warnings for SDK builds. **On Cygwin Clang is assumed to be compiled for Cygwin** and not for
Windows (e.g. downloaded via the Cygwin package manager).

#### C++ support

C++ is supported, just name your sources with `.cpp` extensions. The C++ standard library is not
supported. Exception handling and RTTI are disabled. It's more like C with classes than C++. You'll
need to have `arm-none-eabi-g++` installed to compile C++ code (even if you use Clang - we still
need GCC's headers).

#### Legacy clean

If you're migrating from the old build system to the new one you'll probably want to get rid of
the old cruft. Aside from running `make clean` with an old SDK, you can do it manually:

- For the SDK, remove the `lib` directory and all `.o` files (recursively).
- For APROMs, remove the `bin` directory and all `.o` files (recursively).

#### Misc

The following environment/make variables are available:

- `EVICSDK_MAKE_DEBUG`: set to non-empty to enable extra debug output from the build system.
  Useful when investigating or reporting bugs.
- `EVICSDK_FPU_DISABLE`: set to non-empty to disable FPU support and lazy stacking. Needs a full
  rebuild to avoid mixing FPU and no-FPU objects. Can be useful when debugging very rare, tricky
  FP bugs. Do *not* use this unless you fully understand what it means (no, it won't make you
  binaries smaller or faster, even if you don't use the FPU).

Reporting bugs
--------------

I invite you to report bugs using the Issues page. Please confirm the bug against the latest SDK
commit and include all information needed to reproduce the bug, like:

- OS, make/compiler/binutils/newlib versions;
- Head commit and branch of the SDK you're using;
- Device and flavor (does it happen in release builds? Debug? Both?);
- Code (if applicable), preferably reduced to a Minimal, Complete and Verifiable (MCV) example;
- Things you tried and didn't work;
- For crashes, the crash dump generated by a debug build;
- For build system bugs, the debug output (i.e. `EVICSDK_MAKE_DEBUG=1 make ...`). Try minimizing
  it: if `make evic-dbg` is enough to make it happen, don't send output for `make all`.

Of course if you've already identified the bug in the SDK code you don't need to include as much
information. Be sensible and everyone involved will be happy.

Pull requests
-------------

Pull requests are welcome. A few rules:

- Respect the coding and documentation style of the SDK and refer to the tips & tricks.
- PRs must merge cleanly against the branch they target (preferably rebased on top of the latest
  head for that branch).
- No useless commits. Multiple related commits in the same PR are okay, but no merge/revert commits
  and no "Add foo" "Fix foo" "Fix foo again" stuff. Familiarize yourself with `git rebase [-i]` and
  use it.
- Respect the commit message style (look at the history). The title should tell what the *effect*
  of a commit is, not how it does it. Titles should be imperative: "Add foo", not "Adds foo". For
  non-trivial commits you're welcome to add further technical details in the commit body. Stick to
  80 columns per row.
- If you're fixing a bug [reference it](https://help.github.com/articles/closing-issues-via-commit-messages/)
  in the title. If you're fixing a bug that hasn't been reported yet, create an issue first and
  reference it in your commit title (you don't need to wait for an answer on the issue - go ahead
  with the PR).

It is understood that big or complex changes often won't be perfect, no need to worry about it.
Again, just use common sense and everything will work out.

USB debugging
-------------

The SDK provides a working CDC-compliant USB virtual COM port driver. This allows you to
communicate with a computer for debugging purposes. On Linux and Mac it's plug-and-play. On
Windows, you have to create an INF file with the virtual COM VID/PID pair to get it to install
the driver. An example can be found in the Nuvoton SDK, under
`SampleCode/StdDriver/USBD_VCOM_SinglePort/Windows Driver`.

An example on how to use the port is given in `example/usbdebug`. You can communicate with it
using your favorite serial port terminal. All the line coding parameters (baud rate, parity, 
stop bits, data bits) are ignored, so you don't need to worry about them.

Tips & tricks
-------------

While the SDK does a fairly good job of abstracting the low-level details, you still need to
remember that you're coding on an embedded platform. A few tips that might be helpful:

- You should declare variables shared between threads or with callbacks/interrupts as `volatile`.
- Resources are limited: be frugal. Write efficient code.
- Minimize dynamic memory allocation: all memory not used by data or stack is assigned to heap,
  but RAM is only 32kB and nobody likes a failed `malloc`.
- Declare constant data (such as lookup tables) as `const`: the compiler will place it in ROM,
  reducing RAM usage.
- Prefer `siprintf` over `sprintf`, as it produces much smaller binaries by stripping out the
  floating point printing routines. Of course `siprintf` doesn't support floating point numbers,
  so if you need to print them and cannot use a fixed-point representation you'll have to live
  with the increased binary size.
- When using floating point variables, prefer `float` over `double`, because `float` has hardware
  support. Using `double`s will pull in some floating point emulation code, making your binaries
  larger.
- When using standard floating point functions, prefer the ones with the `f` suffix: hardware
  support for `float`s means faster and smaller binaries. If you use `double` functions, they'll
  pull in tons of floating point emulation code.
