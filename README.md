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

On Windows, first install the [precompiled ARM toolchain](https://launchpad.net/gcc-arm-embedded).
Choose an installation path without spaces to avoid problems with
the build process. Then, install [Cygwin](https://www.cygwin.com/)
and add the following packages on top of the base install:
```
make
git
```

On any OS, you also need a working [python-evic](https://github.com/Ban3/python-evic) install.

**On Cygwin**, hidapi (needed by python-evic) won't build as-is. There are various issues
(Cygwin not recognized as a target, DLL naming conflict, HID open permissions).
Follow those instructions to get python-evic to work on Cygwin:

1. Install the following packages (python, basic build environment, libs and utils):
   
   ```
   binutils
   gcc-core
   gcc-g++
   python
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
   python setup.py install
   ```
3. Download and install python-evic:
   
   ```
   git clone https://github.com/Ban3/python-evic
   cd python-evic
   python setup.py install
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

3. Point the `EVICSDK` environment variable to the `evic-sdk` folder. This should do (assuming your
   current directory is evic-sdk):
   ```
   echo "export EVICSDK=$(pwd)" >> $HOME/.bashrc
   ```
   **On Windows**, you also need to point `ARMGCC` to your ARM toolchain install. Use a Cygwin-style
   path. For example, if my toolchain is installed to `C:\arm-none-eabi-gcc`, I would add this
   to my Cygwin `/home/username/.bashrc` file:
   ```
   export ARMGCC=/cygdrive/C/arm-none-eabi-gcc
   ```
   Make sure to restart your terminal to ensure variables are set before building.

4. Build the SDK:
   ```
   make
   ```

At this point, the SDK should be fully set up. You can also generate Doxygen documentation with:
```
make docs
```
To clean up the build (for example if you want to do a full rebuild), use the standard:
```
make clean
```

Building your first APROM
--------------------------

The `helloworld` example should be the first thing you try compiling and flashing,
to check that everything is working correctly.
Building is as easy as:
```
cd example/helloworld
make
```
To clean you can use `make clean`, as usual.
If the build succeeds, you should now have a `bin/helloworld.bin` file ready to flash.
This file is encrypted and compatible with the official updater.
You can also generate a unencrypted binary:
```
make helloworld_unencrypted.bin
```

Flashing
--------

You can flash the output binary using the official updater. For development,
using `python-evic` is quicker and simpler.
I suggest to backup your dataflash before flashing, in case anything goes south:
```
evic dump-dataflash -o data.bin
```
Now, flash:
```
evic upload bin/helloworld.bin
```
If everything went well you should see the "Hello, World." message.

This APROM doesn't include USB updating, so you need to reboot to LDROM to flash something
else. To do it, remove the battery and disconnect the USB cable. Then, holding the right button,
connect the USB cable. Now you can let the button go and flash away. You can also insert the
battery while the button is pressed, then let it go and connect the cable. I find powering
over USB is more convenient (as long as the APROM doesn't require significant power, i.e.
it doesn't fire the atomizer). Similiarly, holding the left button during powerup will force
the system to boot from APROM.

If `python-evic` fails and the eVic won't flash back to a functioning state, don't panic.
Find a Windows/Mac machine (or virtualize one), boot the eVic to LDROM and flash an original
firmware using the official Joyetech updater. It has always worked for me.

Unless you're messing with the LDROM, this is practically unbrickable - you can always boot
to LDROM and restore. Actually, APROM update is always done from LDROM - the official firmware
doesn't even contain flash writing routines, it only provides access to the dataflash and the
actual APROM upload happens in LDROM after a reset.

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

There are a few caveats, which will be fixed soon:
- Host-to-device communication is not implemented yet (you can't receive data).
