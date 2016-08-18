# PSVCD Introduction.

PSVCD stands for Playstation Vita Cartridge Dump. 
Basically this project summarizes my half year research on the ways to do a hardware dump of PS Vita cart.
I think I should indicate that this research has no relation to Cobra Black Fin project and team.

Currently the process of dumping PS Vita carts is quite involved because you have to create custom board.
On the other hand - this approach does not have any firmware dependency since it is not related to software hacking.
Current version of the board is constructed from DIP elements but I think in the future I will create PCB layout and smaller board.

So the good news are that carts actually can be dumped. You should be able to do it if you complete all the steps of this readme.
At current point I do not know if any of these dumps can be played on multiple different instances of PS Vita.
Most likely it is not possible currently. Process of writing content to PS Vita cart is also not established yet, though I think it can be done.
Also most of the cart content is still encrypted. I do not know any ways to decrypt it now.
Though I am sure that people who work with PSP and PS3 can do it because many files look similar to these consoles.

Last final note. This readme should not be considered as full or complete at current point.
I hope I will be adding more and more details in the future.

# Previous work.

I know that some research was done before me. Unfortunately clear results were not published. 
Previous works include:
- Dump of "Monster Hunter" and "Wipe Out" made my Katsu
- Cobra Black Fin dongle created by corresponding team

There were also some software dumps made by Mr.Gas and game modding/decryption made by Major_Tom.

# Motherboard. Game cart slot schematics.

There can be multiple ways to access game cart data lines on motherboard.
- You can desolder game cart slot and solder to internal pins.
- You can solder to test pads, located on motherboard. 

First of all you will need to disassemble PS Vita body and take motherboard out.
On the opposite side you will see game cart slot. 

Consider looking at pics/pic4.png
This is overall schematic for game cart slot and surroundings with all markings that you will need.

If you want to use first approach - consider looking at pics/pic1.png
Test pads are marked as TP*
Capacitors are marked as C* (though I am not sure they are really capacitors, they can be other smd parts)
Resistors are marked as R* (though I am not sure they are really resistors, they can be other smd parts)
Unknown parts are marked as UNK*

Some guesses on schematics:
C1, C2, C3 - these are most likely supply filter capacitors
C4, R2 - these can be RC circuit for generating game cart insert impulse

If you want to use second approach - consider looking at pics/pic2.png
Marking is similar.

Some info on schematics:
R3, R4, R5, R6, R7 - these are pull-up resistors for DATA and CMD lines.

I have chosen first approach with desoldering game cart slot since at that point I did not know all schematics.
You may see at pics/pic3.png that motherboard also has LED1 and place for one more led that is not soldered.
I have soldered 1x10 2mm female header to the pins because it fits the hole for game cart in PS Vita body.

Finally you will have to create 1x10 2mm male -> 1x10 2.54mm male adapter.
This will be required for further usage of any prototype board.
This adapter can be easily made with some pin headers.
Consider looking at pics/pic5.png to see how it should look like.

# Game cart. Schematics and plugging into prototype board.

I admit that this is not the best approach, so feel free to advice.
It is not good because you have to disassemble game cart.
The best one would be to print game cart slot on 3d printer but I do not own one.
Unfortunately there is no easy way to desolder game cart slot from PS Vita motherboard.
You will definitely need desoldering gun but this is not enough. 
Back side of cart slot is made from plastic and is glued to the motherboard.
Even if you will manage to desolder game cart slot - construction will not be stable.

So I have created simple adapter that can be used with prorotype board.
You will need:
- Game cart
- Memory Stick Duo adapter
- Memory Stick Duo card slot - I desoldered one from old card reader.
- 1x10 2.54mm pin header 

Steps are the following:
- Disassemble Memory Stick Duo adapter
- Solder pin header to adapter
- Disassemble game cart - these come as monolithic chip with memory and controller on board.
- Put game chip under pin headers of adapter then close the adapter.

Consider looking at pics/pic6.png to see how it should look like.

# PS Vita game cart pinout.

Game carts turned out to be simple MMC cards. Pinout is of course a bit different but it still complies with MMC card specification.
Consider looking at pics/pic.png for details.
Game cart has 10 pins which go in the following order:
VCC - 3.3 volt supply
GND - ground pin
CLK - clock pin
DAT3 - data line 3
DAT2 - data line 2
DAT1 - data line 1
DAT0 - data line 0
INS - insertion detect pin
CMD - command pin
GND - ground pin

INS pin must be tied to ground which means that game cart is inserted.
PS Vita uses all 4 DAT lines to transfer data from the cart and also to the cart.
CMD line is used for transferring commands and reading response from the cart.
CLK line would have frequency around 400 Mhz during initialization.
When initialization is finished and reading starts, PS Vita switches to 4-bit width high speed mode.
Not sure though if it is 26 or 52 Mhz but in any case this is very fast signal that is not easy to sample
unless you have professional level gear. Typical logic analyzer will not help here.

Unfortunately FT232h can handle only one input and one output line at a time in MPSSE mode.
Luckily for us game carts support both 1-bit and 4-bit data width modes. 
So by using couple of multiplexers and demultiplexers we can interface with the cart.
Technically FT232h chip is capable of transferring data at max 30 Mhz in MPSSE mode.
I was not able to get to such a high speed. I probably lack some knowledge but the other issue is that
we need properly routed PCB for high speeds instead of DIP prototype board. So high speeds are not expected to work at the moment.
Again luckily for us game carts can be accessed on low speeds for reading as well.

# Configuring FTDI FT232h.

At this point you should be able to connect PS Vita and game cart to any prototype board.
Next step would be creating custom board that will allow to dump game card.
But before that we will need to configure FTDI chip that serves as heart of the board.

For custom board you will need FT232h chip. 
I guess FT2232h can also be used - it just has more pins and two channels instead of one.
FT232h can come on a breakboard - Adafruit FT232H, FTDI UM232H etc. You can use custom board as well. 

Before configuring FT232h chip you have to install D2XX direct drivers. They can be found on FTDI web page.

For configuring FT232h we will need FT_Prog utility that can be downloaded from FTDI web page.
Settings for FT232h are as following:

- USB Config Descriptor -> Self Powered: Enabled.
  We will use self powered configuration because all parts of the custom board including FT232h will be
  powered from one voltage regulator. I found out that some FT232h chips do not give 3.3 volts 
  on corresponding 3v3 pins when USB power mode is selected. We MUST use 3.3 volts or game cart can be damaged.
  
- USB String Descriptors -> Product Desc: USB FIFO.
  This is an identifier for the device that will be later used in C++ code. It will come in handy if you use
  other FTDI devices that are also plugged into your PC. 
  For example I also used Lattice MachXO2 FPGA for some investigations. And it has FT2232h chip on board.

- Hardware Specific -> Suspend on ACBus7 Low: Enabled.
  This setting is required if FT232h is used in self powered mode. ACBus7 line must be tied to 5V through resistor.
  It will be explained later.

- Port A -> Driver -> D2XX Direct: Enabled.
  We will use fast direct drivers and not virtual com port.

- Port A -> Hardware -> 245 FIFO: Enabled.
  This setting is required when Sync FIFO mode or MPSSE mode is used.

## Troubleshooting.

Problem 1.

There can be cases when you will not be able to program FT232h chip. In most cases you will see this error:
"Index was outside the bounds of the array".
Unfortunately this is a bug in FTDI software that is mixed together with flaw in your breakboard.
When you observe this error take a closer look at your EEPROM. Most likely you will have 93C46 type.
It is indicated in FTDI datasheets that 93C46 can be used with FT232h but unfortunately it can not.

There are 2 solutions for this problem.
- FT_Prog is written in C#. You can decompile it with Reflector and fix the code a bit so that FT232h can be programmed even with smaller 93C46.
  I have this fix so maybe I will share it in the future.
- Desolder 93C46 EEPROM and solder 93C56 EEPROM or bigger one.

Problem 2.

Some FT232h chips do not give 3.3v on corresponding pins when used in USB powered mode. 
They may give 3.6v or 3.8v etc. To fix this you have to reconfigure FT232h chip to work in self powered mode.
This may require some hadrware changes as well.

For example on my breakboard I had to desolder Zero Ohm resistor that connected USB pin and VREGIN pin.
Then I have soldered another 39K Ohm resistor that was connected to AC7 and to USB pin.
Please consider looking into FTDI datasheets to understand what is required for self powered configuration.

It looks like that famous Adafruit breakboard also has Zero Ohm resistor.
FTDI UM232H is more clever - you can use jumpers without desoldering anything.

# Building custom prototype board.

Heart of all system is a custom board that allows to interaction between PC and PS Vita game cart.
Consider looking at schematic pics/pic7.png for further details.
Custom board consists of multiple sections that are described below.

## Voltage regulator section.

When powered from USB we have 5 volts. Any SD cards or MMC cards work from 3.3 volts or even lower 1.8 volts.
Using 5 volts will damage the card. 
Voltage regulation section is located in top left corner of schematic file.

Required parts are:
- DS1099-B USB port type B. You can use Type A if you prefer. 
- Two leds: these are optional and are used just to show that power is on.
- R21, R22: 220 Ohm resistors for leds.
- LD1117V33: Voltage regulator that will transform USB 5V to 3.3V
- C1: Filtering capacitor 100 uF.
- C5, C6, C7: Filtering capacitors 1uF.
- S1: Dip Switch 1 pin: this is optional. Used to switch custom board power on and off.
- SV3, SV4: Two 1x10 2.54mm female headers. These are used to wire any other places of custom board to VCC3V3 or GND.

## Data lines section.

This section contains multiple female headers that can be used for wiring different devices.
There are also other components that can be used to configure each individual line.
Section can be found in the middle left part of schematic file.

Required parts are:
- Two 2x10 2.54 male pin headers. These are used in conjunction with jumpers for each data line.
- Two 2x10 2.54 female headers. These are used to connect any external devices (game cart, logic analyzer etc) or to do internal wiring.
- JP1, JP2, JP3, JP4, JP5, JP6, JP7, JP8, JP9, JP10. These are jumpers that can be used to pull
  each individual data line to VCC3V3.
- R1, R2, R3, R4, R5, R6, R7, R8, R9, R10. These are 4.7K Ohm pull-up resistors.
- JP11, JP12, JP13, JP14, JP15, JP16, JP17, JP18, JP19, JP20. These are jumpers that can be used
  to pull each individual data line to GND.
- R11, R12, R13, R14, R15, R16, R17, R18, R19, R20. These are 4.7K Oh, pull-down resistors.

## Data multiplexing section.

This section is used to select 1 of 8 data lines and feed output to FT232h chip.
Section can be found in the bottom left part of schematic file.
Data transfer between game cart and PS Vita is bidirectional. 
That means that we may use tristate buffers as well. Though buffers are optional.
Output of multiplexer can be controlled with G (enable) signal.

Required parts are:
- 74HC244N: Octal tristate buffer.
- 74HC151N: 8-dine to 1-dine data multiplexer.

Main idea is to connect data lines D0-D7 to 74HC244N. Output of 74HC244N should be connected to 74HC151N.
74HC151N will allow to select one of 8 data lines. Others will be tristate.
Address lines of 74HC151N are connected to GPIO pins of FT232h.
Enable pins of 74HC244N should be connected to GPIO pin of FT232h.

This will allow to select and read 1 of 8 data lines while others will be tristate. 
Address of line and read/write mode will be controlled by FT232h.

## Data demultiplexing section.

This section is used to select 1 of 8 lines and use it as output to game cart.
Section can be found in the bottom right part of schematic file.

Required parts are:
- Two 74HC125N: Quadruple bus buffer gates with tristate outputs.
- 74HCT138N: 3 to 8 line decoder, inverting

Main idea is to connect data lines D0-D7 to output pins of 74HC125N tristate buffers.
Outputs of 74HCT138N decoder should be connected to enable pins of 74HC125N.
Input pins of 74HC125N should all be connected together to GPIO pin of FT232h
Address lines of 74HCT138N decoder should be connected to GPIO pins of FT232h.
Enable pin of 74HCT138N should be connected to GPIO pin of FT232h.

This will allow to select and write to 1 of 8 data lines while others will be tristate. 
Address of line and read/write mode will be controlled by FT232h.

## Game cart initialisation bypass section.

PS Vita game cart requires special initialization sequence before game cart can be read.
This sequence can not be reproduced at current point. Though it is already partially known.
To bypass this initialization a simple trick can be used:
- Allow real PS Vita to initialize game cart. 
- Then connect custom board. 
- Then disconnect PS Vita.
- Now you can read game cart.

Make sure that during these steps game cart is always powered.

Required parts are:
- SV5, SV6, SV7: Three 1x10 2.54mm female headers.
- S2, S3: Two 1x10 Dip switches.

Main idea is to connect PS Vita data lines to SV6 and custom board data lines to SV5.
On the other hand game cart will be connected to SV7.

## Custom board core section.

Core of the custom board is FT232h chip.
There are some other components required so that Ft232h runs as expected.

Required parts are:
- R23, R24, R25, R27, R28, R29, R30, R31, R32: 4.7K Ohm pull-up/pull-down resistors.
- Four 1x2 2.54mm male pin headers. 
- JP21, JP22, JP23, JP24: jumpers for selecting pull-down or pull-up configuration.
- R26: 39K Ohm pull-up resistor.

Main idea is to connect address lines of 74HC151N to GPIOH pins AC0-AC2 of FT232h.
Address lines of 74HCT138N decoder should be connected to GPIOH pines AC3-AC5 of FT232h.
GPIOH pin AC6 serves as read/write mode pin and should be connected to 74HC244N buffer and 74HCT138N decoder.
Single pin can be used because 74HC244N and 74HCT138N use different levels of enable signal.
74HCT138N uses high and 74HC244N uses low.
All address lines and R/W line can be pulled together either to GND or to VCC by using jumpers JP23 or JP24. 
Due to properties of my prototype board I was not able to solder separate jumper for each line. But you can do it.

GPIOH AC7 line should be connected to USB 5V pin of FT232h through 39K Ohm pull-up resistor.
GPIOH AC8, AC9 pins can not be used in MPSSE mode of FT232h so they are not connected.
MPSSE mode will be explained later.
VREGIN pin of FT232h should be connected to 3.3 volt output of voltage regulator.

3V3, VCCIO pins should be connected to 3.3 volt and two GND pins should be connected to ground.

GPIOL AD0 pin will serve as CLK in MPSSE mode.
GPIOL AD2 pin will serve as DIN in MPSSE mode so it is connected to 74HC151N output.
GPIOL AD5 pin will serve as DWAIT in MPSSE mode so it is connected to 74HC151N output also.
GPIOL AD1 pin will serve as DOUT in MPSSE mode so it is connected to input of 74HC125N buffers.
GPIOL AD3 pin can only serve as CS in MPSSE mode so it is not used and not connected.
GPIOL AD4, AD6, AD7 pins are not used so they are not connected.

DOUT, DIN/DWAIT lines can be pulled together either to GND or to VCC by using jumpers JP21 or JP22.

## Why FT232h. MPSSE Mode Description.

Choice of FTDI FT232h chip was not spontaneous. There are two main reasons for this:
- It allows easy interfacing with other devices through USB
- It allows to implement custom protocol with special MPSSE mode if having standard interfaces like UART, FIFO, SPI is not enough for you.

Basically MPSSE mode is the special mode of FT232h where you send commands to the chip through USB and they are translated into signals.
Main features that may be useful for custom protocol include:
- Single CLK line
- Single DIN line
- Single DOUT line
- Single Wait line
- 3 GPIO Low pins
- 7 GPIO High pins

Using this MPSSE mode I have implemented variation of MMC protocol which uses 1-bit width bus and low frequency mode to transfer the data in both directions.

I also use FT232h in FIFO mode together with FPGA when I need to investigate packets on the data line.
And there are many other ways FT232h can be used as well.

# Interconnections. Connecting PS Vita, game cart and custom board.

Most of the custom board connections are soldered. But I tried to keep custom board as much generic as I could.
So I left out some interconnections that should be connected with dupont wires. This allows more flexible configuration.

Consider looking at pics/pic7.png for further details.

There are two possible ways to connect:
- Connect game cart directly to female pin header SV1 or SV2. This approach can be used only if CMD56 sequence is known (explained later).
- Connect game cart to female pin header SV7. Then connect PS Vita to female pin header SV6 so that it can be enabled/disabled by using DIP switch S2.
  Finally connect female pin header SV5 with female pin header SV1 or SV2 so that all functionality of custom board can be enabled/disabled by using DIP switch S3.

There is some additional wiring required as well.
- Use female pin header SV3 to connect VCC line with corresponding pin of the game cart. You can use SV1 or SV2 for this.
- Use female pin header SV4 to connect GND line with corresponding pins of the game cart. You can use SV1 or SV2 for this.
- Connect CLK pin of FT232h with corresponding pin of the game cart. You can use SV1 or SV2 for this.
- From jumpers JP1-JP10 select those that you have used for CLK, CMD and DAT0 lines. Place these three jumpers to pull-up the lines.

Consider looking at pics/pic9.png which shows final result.
You may notice one difference to the schematic - WAIT-DIN wire should not be there.
Basically I did not solder this two wires together so I had to place a connection wire.

Final connection with PS Vita wired together is shown on picture pics/pic10.png
You may notice that I had connected everything to the prototype board. 
Basically I did not have 1x10 DIP switches so I had to manually switch wires.

# PS Vita game cart protocol.

I was kinda surprised when I figured out that standard MMC protocol is used to interface with game carts.
Specification for this protocol can be found at JDEC web site.
When game cart is inserted and PS Vita is turned on - cart initialization starts. 
You may notice this by looking at flashing red led indicator.

Initialization of PS Vita game cart consists of 3 steps.

## PS Vita game cart initialization - Step 1.
During this step PS Vita tries to identify standard SD card or SDIO device.
This is of course a speculation but a possibility can exist that PS Vita can interface with SD cards or SDIO devices.
Commands that are sent during this step are not required to initialize MMC card.
So I do not see any other logical explanation for these commands.

Initialization consists of these commands:
- 40 00 00 00 00 95 - CMD0 - GO_IDLE_STATE
- 48 00 00 01 AA 87 - CMD8 - SEND_IF_COND
- 45 00 00 00 00 5B - CMD5 - IO_SEND_OP_COND
- 77 00 00 00 00 65 - CMD55 - APP_CMD

## PS Vita game cart initialization - Step 2.
During this step PS Vita tries to initialize PS Vita game cart which is basically MMC card.
I will give a very brief description of what is happening. I am planning to share more details on this later.

Initialization consists of these commands:
- 40 00 00 00 00 95 - CMD0 - GO_IDLE_STATE
- 41 40 FF 80 00 0B - CMD1 - SEND_OP_COND
- 42 00 00 00 00 4D - CMD2 - ALL_SEND_CID
- 43 00 01 00 00 7F - CMD3 - SET_RELATIVE_ADDR
- 49 00 01 00 00 F1 - CMD9 - SEND_CSD
- 47 00 01 00 00 DD - CMD7 - SELECT_CARD
- 46 03 AF 01 00 43 - CMD6 - SWITCH (ERASE_GROUP_DEF)
- 48 00 00 00 00 C3 - CMD8 - SEND_EXT_CSD
- 50 00 00 02 00 15 - CMD16 - SET_BLOCKLEN
- 46 03 B9 01 00 2F - CMD6 - SWITCH (HS_TIMING)
- 46 03 B7 01 00 2D - CMD6 - SWITCH (BUS_WIDTH 4)

## PS Vita game cart initialization - Step 3.
During this step PS Vita executes custom initialization sequence through generic command CMD56.
This step is required for initialization. If it is not executed game cart can not be read.
CMD56 is generic command that allows PS Vita to interface with game cart by sending and receiving data packets.
Format of these packets is vendor specific. So basically this step is a game cart protection step which identifies only original game carts.

This initialization happens at high speed and is hard to sample unless you own professional gear. I do not own one so I have tried to sample with FPGA instead.
As far as I understand this step includes 10 pairs of request/response packets.
Some of these packets are constant while others are not and partially change after each time you turn on PS Vita.
This is most likely related to some encryption mechanism.

If you want to completely reproduce initialization of PS Vita game cart then understanding of Step 3 is essential.
This may be required if you want to try using SD card / MMC card / SDIO external device that contains game dump instead of original game cart.
Basically some additional hardware will be required to simulate this CMD56 initialization step.

Last final comment. I currently have the dumps of these 20 data packets but I am not going to share them in the nearest future.
I need to understand the structure of these packets and be sure that they do not contain any personal information that can disclose the user.

Initialization consists of these commands:
- 78 00 00 00 00 25 - CMD56 (REQUEST)
- 78 00 00 00 01 37 - CMD56 (RESPONCE)

# Reading PS Vita game cart.

Consider looking at pics/pic7.png for details.

So it looks like game carts have protection and there is no way to deal with it.
Lucky for us very old trick can be used to bypass this protection. Steps are the following:
- Connect PS Vita and game cart - use SV2 DIP switch.
- Turn on PS Vita and let it initialize the cart.
- Connect custom board. Make sure that custom board also supplies power to the cart - use SV3 DIP switch.
- Disconnect PS Vita - use SV2 DIP switch. Make sure that at this point PS Vita did not go to sleep mode and screen is still turned on. 
  When PS Vita goes to sleep mode it sends deinitialization sequence to the game cart.
  
At this point game cart is initialized but we need to send couple more commands before we can read the data.
First command is CMD6 [46 03 B9 00 00 39] - this switches game cart to low speed mode. Technically you should get a response for this command.
But custom board is not able to handle high speeds so basically this response will contain garbage.
Second command is CMD6 [46 03 B7 00 00 3B] - this switches game cart to 1-bit width data mode from 4-bit width data mode that is used by PS Vita.
This command should give valid response since we are now in low speed mode.

At this point you should be able to read PS Vita game cart.
Basic commands that can be used include:
- CMD17, CMD13 - reads single sector, check cart status.
- CMD23, CMD18, CMD13 - set number of sectors, read multiple sectors, check cart status.

These are exact sequences that are used by PS Vita to read game carts.

Last comment. I was also able to sample write sequences so probably it is possible to write to game carts.
Writing sequence includes:
- CMD24, CMD13 - write single sector, check cart status.

# PS Vita game cart filesystem.

Surprisingly game cart filesystem is EXFAT. There are no official specifications published but some documentation on this filesystem can be found on net.
Filesystem is not complicated and can be parsed without any issues. I have created corresponding parser tool that takes raw cart dump as input and
produces directory structure with all the files from the cart in specified location.

There is one exception that I should mention. EXFAT filesystem is not located at sector zero.
Instead at sector zero you will find some SCEI specific structure that points to EXFAT VBR record.

# Directory structure.

The cart that I was trying to dump was "SAO - Hollow Fragment".
Directory structure looks as following (will fix this list later. is is displayed incorrectly):

- root
 - app
  - PCSG00294
   - sce_module
    - libc.suprx
    - libfios2.suprx
    - libult.suprx
   - sce_pfs
    - files.db
    - unicv.db
   - sce_sys
    - about
     - right.suprx
    - livearea
     - contents
      - bg.png
      - frame1.png
      - gate.png
      - template.xml
    - manual
     - 001.png
     - ..
     - 035.png
    - package
     - body.bin
     - head.bin
     - stat.bin
     - tail.bin
     - temp.bin
    - trophy
     - NPWR05519_00
      - TROPHY.TRP
    - clearsign
    - icon0.png
    - keystone
    - param.sfo
    - pic0.png
   - data.psarc
   - eboot.bin
   - hud_settings.ini
 - gc
  - param.sfo
 - license
  - app
   - PCSG00294
    - ___.rif
 - psp2 
  - update
   - psp2updat.pup
 
# File encryption/compression.

Most of the files are encrypted/compressed - I am not sure which.
So most likely this dump can not be easily used on different PS Vita instances.
I only obtained valid dump recently and did not have time yet to explore the files.
I am quite sure that encryption scheme should be quite identical to ps3.
So most likely people that are experienced in that area will be able to figure out the details much quicker.

# Building the project.

## Installing D2XX direct drivers.

Unfortunately current implementation is only for Windows platform.
This restriction is based on FTDI drivers that are for Windows.
I think they also have some drivers for Linux as well so I am going to figure this out in the future.

After drivers are installed - create two environment variables:
- FTDI_INCLUDEDIR : this should point to the folder where ftd2xx.h is located
- FTDI_LIBRARYDIR : this should point to the folder where ftd2xx.lib is located

These two variables will be used by cmake later.

Also do not forget to add path to FTDI dlls to your PATH variable

## Installing or Compiling BOOST.

This is optional and not required for dump utility. However this is required for exfat parser utility.
I am not sure if I should quit using boost since it is not very easy to compile and may be considered big dependency to drag.
But it has many useful things in it which I am used to.

You may either install prebuilt binaries or compile by yourself. There are lots of guides for this.
If you are going to compile from source then make sure you compile static multithreaded version of boost.
After you have your installation - create two environment variables:
- BOOST_INCLUDEDIR : this obviously should point to the directory where "boost" directory with all the includes is located.
- BOOST_LIBRARYDIR : this variable should point to the directory with boost static libraries.

These two variables will be used by cmake later.

## Installing cmake.

Cmake is very useful crossplatform tool for generating makefiles/solutions etc.
You can obtain installation from their web page.

## Generating solution.

All you need is to navigate to src folder and execute generate.bat
It will invoke cmake and generate MSVC 12 solution.
If you want to use different version of MSVC - just change generator name that is specified in the bat.
You can check generator names by calling cmake help.

## Compiling.

Basically at this point you should be able to compile the project. 
Let me know if there are any issues.

# Brief description of tools and libraries.

There are three executables and one library in total:
- common : this library contains basic functionality that is required to initialize FTDI chip. 
           It also contains low level stuff that uses driver API to talk directly to the chip.
- sd_card : this one is currently a prototype for experimenting with something different than MMC cards. Not important.
- mmc_card : this one can do several things with PS Vita game cart including:
 * standalone initialization : this is currently a prototype because I need to figure out what to do with CMD56.
 * entering dumpable state : this executes sequence of commands that prepare PS Vita game cart for dumping using FTDI chip and custom board.
 * dump PS Vita game cart : this basically starts dumping PS Vita game cart to binary file. 
   This is a long process and it can be interupted by transmission errors. However you can continue dump after failure by specifying different cluster address.
   Different destination file can also be specified if you wish to manually merge parts of the dump. For example in WinHex.
- dump_exfat: this one basically takes raw dump of PS Vita game cart and extracts all the directory hierarchy and the files to specified location.

# How to dump PS Vita game cart and get the files.

After you have bypassed CMD56 initialization step by using DIP switches do the following.
- Run mmc_card executable with mode = 0. This will switch game cart to 1-bit bus low frequency mode.
- Run mmc_card executable with mode = 1. Specify destination binary file path and start cluster address (should be zero on first run).
- Wait. Seriously wait. Dump can take around 20 hours for 3.5 GB of data. I do not have exact time metrics but I am going to measure total time that can be required for dump.

When dump is finished you should obtain binary file that is basically snapshot of game cart filesystem.
Pass this file to dump_exefat executable and specify destination directory path. 
This tool then should extract all the files and create corresponding directory structure in the destination folder.
