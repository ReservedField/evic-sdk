eVic SDK is a software development kit for writing APROMs for the Joyetech eVic VTC Mini.

Installation
---------------

1. You need to setup an arm-none-eabi GCC toolchain and newlib.
   On Fedora, install the following packages:
   ```
   arm-none-eabi-gcc
   arm-none-eabi-newlib
   ```
   On Ubuntu, the following packages should be enough:
   ```
   gcc-arm-none-eabi
   libnewlib-arm-none-eabi
   ```
   I only tested it on Fedora at the moment, but there shouldn't
   be issues with other distros. In case the precompiled packages
   aren't available for you distro, you may have to compile it yourself.
   I still haven't tested it on Windows.

2. Clone this repository:
   ```
   git clone https://github.com/ReservedField/evic-sdk.git
   cd evic-sdk
   ```

3. Download the latest [M451 series SDK](http://www.nuvoton.com/hq/support/tool-and-software/software)
   from Nuvoton and copy the `Library` folder inside `evic-sdk/nuvoton-sdk`, as to have
   `evic-sdk/nuvoton-sdk/Library`.

4. Build the SDK:
   ```
   make
   ```

5. Point the `EVICSDK` environment variable to the `evic-sdk` folder. Tipically, you'll add
   this to your `.bashrc` file in your home directory:
   ```
   export EVICSDK=/path/to/evic-sdk
   ```
   Make sure to restart your terminal to ensure the variable is set before building APROMs.

At this point, the SDK should be fully set up. You can also generate Doxygen documentation with:
```
make docs
```
To clean up the build (for example if you want to do a full rebuild), use the standard:
```
make clean
```

Display driver
--------------

The eVic VTC Mini uses either the SSD1306 or the SSD1327 display controller, depending
on the hardware version. The following hardware versions use the SSD1327:
- 1.02
- 1.03
- 1.06
- 1.08
- 1.09
- 1.11

All the other use SSD1306.

Since dataflash support isn't built in at the moment, you have to manually select the display.
To do so, edit the `LCD_TYPE` define in `src/include/Display.h`. Set it to `0` if you have a
SSD1327 display, or to `1` if you have a SSD1306.

If you set the display type correctly but it still doesn't work, change the `TODO_DATAFLASH`
define in `src/display/Display_SSD1306.c` or `src/display/Display_SSD1327.c` from `1` to `0`.

After altering those files, you have to rebuild the SDK and then the APROM.

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

Flashing
--------

To flash, use [python-evic](https://github.com/Ban3/python-evic). I suggest to backup your
dataflash first, in case anything goes south:
```
evic dump-dataflash -o data.bin
```
Now, flash with APROM verification disabled:
```
evic upload --no-verify aprom --unencrypted bin/helloworld.bin
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
