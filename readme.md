# Color SPU Strip

A low-cost, high-sensitive sound pickup light strip.<br>
Using MEMS mics, STM32G0, WS2812B, etc.<br>

The cost of the whole hardware items are no more than 10 RMB(current design costs 7.1 RMB)<br>

<img src="monazite-logo-lofi.png" width=80><br>
Author: jlywxy(jlywxy@outlook.com)<br>
Document Version: 2.1 (2023.2.1)<br>

- --

Current design: Hardware - v1.1r1, Software - v2.0r1.<br>
<img src="demo.jpg" width=400/>

<b>The Circuit Schematics, PCB drawings with their PCB project(LCEDA), and software source codes are all open-sourced until now.</b>

- --

## Main Function
1. Sound Pickup Light strip<br>
The light strip can "hear" the loudiness of the environment, then convert the loudiness to the length of the light. Moreover, it can change color when high frequency of sound appears.<br>
There are two modes of the strip: light coming from the side, or coming from the middle of the strip.
2. Color-gradient Light strip<br>
The light strip shows pure color of lights that changes with gradient, like the rainbow.
2. Auto sleep<br>
When in sound pickup mode, if the loudiness is too small in a region of times, it shuts down temporarily. When a loud sound comes, it can auto wake up.

- --

## Hardware Design

1. BOM List<br>
```
Part No.                   Count   Brand
----------------------------------------------
STM32G030F6P6              1     ST
SPH0641LM4H/SD18OB261-060  1     Knowles/Goertek
XC6206A(3.6v)              1     -
USB Type-C Connector(16p)  1     -
WS2812B                    17    worldsemi/-
100nF 0402 Capacitor       22    -
5.1k Ohm 0603 Resistor     2     -
0 Ohm 0603 Resistor        1      -
Diode SMA                  1      -
3*4*2.5 Click Button       1      -
```
- --
Note: 
* SPH0641LM4H(Knowles) and SD18OB261-060(Goertek) are pin-to-pin compatible but not tested in this project.<br>
* "-" means any brand equivalent.<br>
## Product Specification

<-- To Be Continued -->

- --

## Functional Patch

Color SPU Hardware: <br>
See comments in circuit schematics.<br>

Color SPU Software:<br>
2.0: <br>
* Finished mode transition functions. <br>

2.0r1: <br>
* fixed auto-wakeup to default mode.<br>
* added brightness gradient animation.<br>
* modified auto-wakeup thershold to 11.<br>