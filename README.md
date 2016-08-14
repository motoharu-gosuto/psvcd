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

# Configuring FTDI FT232h.

At this point you should be able to connect PS Vita and game cart to any prototype board.
Next step would be creating custom board that will allow to dump game card.
But before that we will need to configure FTDI chip that serves as heart of the board.

For custom board you will need FT232h chip. 
I guess FT2232h can also be used - it just has more pins and two channels instead of one.
FT232h can come on a breakboard - Adafruit FT232H, FTDI UM232H etc. You can use custom board as well. 

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

## Troubleshooting

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

# Building custom prototype board



