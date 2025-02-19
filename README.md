# ABS2SPEED
It is a simple electronic circuit that allows to control the vehicle speedometer with recalculated pulses from ABS sensor. This is useful but not limited to situations like:
- the original gearbox was swapped for one that doesn't have vehicle speed sensor (impulsator),
- original speedometer was using mechanical wire but was swapped for electronic one and gearbox cannot be equipped with a proper vehicle speed sensor (impulsator),
- the speedometer was swapped for a different model not compatible with signal from vehicle speed sensor,
- the size of wheels is significantly different from the original and speed indication is far from accurate,
- direct wheel speed sensing is faster/more responsive than GPS based speed,
- alternatively can be used with tachometer for example when it is designed for a different number of cylinders.

>[!IMPORTANT]
>The circuit is based on Arduino Nano V3 equipped with ATMega328PB microcontroller. It will NOT WORK with modules populated with ATMega328P!

![Prototype](A2S_v3.1/DStage_A2S_v3.0_proto1.jpg)

## Input
ABS2SPEED should work with various input types. Here are tips for setting the input up:
 ### 1. 3-wire Hall effect sensor
 - sensor with separate power and signal output,
 - sometimes used as impulsators in gearboxes,
 - connect signal to "HA" input and ground to "GND"
 - you can power the 5V sensor with the "5V" output,
 - if the output is open-collector type (requires pull-up) set the "MODE" jumper in "PU" position,
 - usually the output square wave will have the low level close to 0V and high level close to 5V,
 setting the potentiometer "PR1" to about 2.5V should work fine (middle position, can be measured at the test point "TP1").
 ### 2. 2-wire Hall effect sensor
 - sensor with indirect power mixed with output signal,
 - used as ABS sensors on vehicles with a magnetic path or magnetic ring on the wheel hub (for example Fiat Seicento, BMWs),
 - connect the signal/ground wire to "HA",
 - power the sensor with the "10V" output,
 - set the "MODE" jumper in "PD" position to enable indirect power
 - the range of the output square wave can vary,
 as a ballpark set the potentiometer "PR1" to about 2.5V (middle position, can be measured at the test point "TP1").
 setting the potentiometer "PR1" to about 2.5V should work fine (middle position, can be measured at the test point "TP1").
 ### 3. VR sensor
 - variable reluctance sensor, 
 - used as ABS sensors on vehicles with a metal crown wheel on the wheel hub,
 - connect one sensor wire to "VR+" input and the other to "GND",
 sensor is not polarized so usually it won't matter which wire goes where,
 - set the "MODE" jumper in "VR",
 - the amplitude of the output sine wave is frequency (vehicle speed) dependent,
 therefore it is usually best to look for the zero crossing of the signal by setting the potentiometer "PR1" to about 0V (all the way left, can be measured at the test point "TP1").
 ### 4. Direct square wave signal
 - for example for use with speed or tacho signal directly from ECU or other unit, 
 - connect the signal to "HA" input and ground to "GND",
 - connect one sensor wire to "VR" input and the other to "GND",
 - if signal is open-collector type (requires pull-up) set the "MODE" jumper in "PU" position,
 - set the potentiometer "PR1" around the mid-point of the signal (can be measured at the test point "TP1"),
 in usual cases that's either 2.5V (5V signal) or 5V (12V signal like when pull-up is in use).

When input is set up correctly pulses from the sensor will be indicated by "D4" LED.

## Output
The input of the speedometer (or tachometer) is to be connected to the "OUT" terminal. The output is pulled to ground with a transistor and has a 10k pull-up to 12V. 
This should work with most gauges but please note that some may require removal of the "R22" pull-up.
The output pulses are indicated by D5 LED as well as the LED on Arduino Nano module.

## Frequency recalculation factor
By default the recalculation factor is set by the 10 position switch "S1" (or jumpers used in its place). The factor can range from 0.01 up to 10.23. 
Each position of the switch has a signed factorial weight and the weights are added together to form the final factor. Starting from most right:
 - position 1–5.12
 - position 2–2.56
 - position 3–1.28
 - position 4–0.64
 - position 5–0.32
 - position 6–0.16
 - position 7–0.08
 - position 8–0.04
 - position 9–0.02
 - position 10–0.01

Please note the the factor is read from the switch only at the start of the program. In order to refresh the factor (apply new setting) the circuit has to be either power down and up again or reset by hitting the reset button on the Arduino Nano module.

The factor can be also hard-coded in the program by modifying the "USE_DIPSWITCH" parameter to "0" and setting the factor in HARDCODED_FACTOR parameter.

Factor can be determined experimentally but also precalculated. Here's an example:
 - vehicle wheels circumference is 1.65m (calculated from the rim and tire size),
 - the ABS magnetic path on the wheel hub has 96 magnetic poles (48 S and 48 N),
 - from the above the ABS sensor frequency at 100km/h is:
 100km/h = 100000m / 3600s = 27,78m/s
 27,78m/s / 1.65m = 16,835 rotations/second
 16,835 * 48 pulses/rotation = 808Hz @ 100km/h
 - speedometer requires 525Hz to indicate 100km/h
 - recalculation factor needs to be 808Hz / 525Hz = 1.54

## Additional functions and settings
### Speedometer limit
 If the vehicle speed can go higher than speedometer scale some of the speedometers can experience problems such as moving the needle all the way down to 0, some erratic behaviour or other unwanted effects.
 For such cases user can set the speedo limiting frequency in the program. For that the parameter "USE_SPEEDO_LIMIT" has to be set to "1". The limitation is set by "MAX_SPEEDO_FREQ" parameter.
 If you don't know the limit frequency it can be determined experimentally with a help of the second special function - see point 2.

### Staging
 The "Staging" sometimes called "Opening ceremony" is the gadget function that will move the needle all the way up the scale and then down to 0 when vehicle is started.
 To enable this option please set "USE_STAGING" parameter to "1". It is important to also set the "MAX_SPEEDO_FREQ" correctly, so the needle hits end of the scale accurately.
 For pleasing aesthetic effect the delay parameters "PRE_STAGING_DELAY", "MID_STAGING_DELAY" and "POST_STAGING_DELAY" should be tweaked.

 If the limiting frequency (end of the scale) is not known it can be determined experimentally. Set the "MID_STAGING_DELAY" to a relatively large value such as "7000" (7s delay) so the needle stays in the top position long enough to spot it. Then modify the "MAX_SPEEDO_FREQ" parameter and reset or power down-up the circuit to see the effect until the correct value is found.

## HW ordering and assembly
ABS2SPEED PCB can be ordered preassembled from JLCPCB. You will need 3 files: gerbers, BOM and CPL (Pick'n'Place) and just follow the process on JLCPCB website. In case of issues please contact me on dstagegarage@gmail.com. 

Please note that by default the throug-hole parts are not included in the Bill of Materials (BOM) and have to be purchased and soldered manually by the user. This includes: 
- 4 pieces of double 5mm screw terminals,
- 10 position 2.54mm dip-swith or 2x10 2.54mm jumper header (gold pin),
- 2x3 2.54mm header for "MODE" jumper (gold pin),
- 2 pieces of 1x15 female 2.54mm headers for Arduino Nano (optionally can be soldered directly to the board),
- Arduino Nano V3 module (has to be based on ATMega328**PB**, NOT ~~ATMega328P~~).

![Prototype](A2S_v3.1/DStage_A2S_v3.0_proto2.jpg)

## Programming Arduino module
Please follow the instructions in [SW/Readme.md](https://github.com/DStageGarage/ABS2SPEED/tree/main/SW/Readme.md).

## Casing
The model and STL file for a 3D printed case will be available on request or when I have time ;-).
