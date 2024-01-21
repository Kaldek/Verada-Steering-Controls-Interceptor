# Verada-Steering-Controls-Interceptor
Mitsubishi Verada/Diamante steering wheel radio control interceptor for modern head units.

## Project Overview
The Mitsubishi Verada/Diamante from the late 1990s through to the early 2000s uses steering wheel radio controls which are massively over engineered and don't work with moder head units without some form of translation circuit.  Rather than buy one, I decided to reverse engineer how the system works and build one.

### Issues with the Mitsubishi steering wheel controls

#### What Mitsubishi did NOT do
Even modern (2020+) vehicles often still use a "resistor network" in the steering wheel.  As buttons on the steering wheel are pressed, each button has its own corresponding resistor.  Using a voltage divider, it is possible to determine the differences in resistance for each button.  Most aftermarket head units that support steering wheel controls can be connected directly to these resistor networks and just be programmed in the head unit setup menu to match each button press to a head unit function.

#### Mitsubishi's approach
In typical Japanese fashion, Mitsubishi over-engineered their steering wheel controls and used a button matrix for all six steering wheel buttons, connected to an NEC PD6134 IR remote controller chip.  This chip's functionality is only partially used; button presses are reflected out one of the chip's I/O ports rather than the IR "Remote" driver pin.  This removes all of the IR 38khz carrier signal and just generates digital pulses.  These pulses are inverted through a series of MOSFETs to pull down the steering wheel signal wire each time a pulse is generated.

The signal wire from the steering wheel is shared with the signal used by the cruise control stalk, which **does** use a resistor network.  So, the single signal wire is used for carrying a mix of serial data pulse pullowns, and resistance changes as measured by the cruise control unit.

### Solution Summary
The solution which I have created here uses An Arduino to connect to the signal wire of the radio control module in the steering wheel.  Pulses are detected based on the reverse engineered timings of the pulse frequencies to decode the signals, of which there are only six unique varations, likely based purely on the button matrix decoding which the PD6134 performs.
