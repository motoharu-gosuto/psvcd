# psvcd

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
I have soldered 1x10 2mm header to the pins because it fits the hole for game cart in PS Vita body.













