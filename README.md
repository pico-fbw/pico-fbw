# pico-fbw

A fly-by-wire system for RC airplanes designed to run on the Rasperry Pi Pico microcontroller.

This is still very much a work in progress/alpha project! Feel free to bookmark this and keep up on development, and be sure to let me know of any suggestions you may have. Thanks for stopping by!

## Development Goals

- [x] At least 2, maybe 3 control modes
  - [x] Direct: inputs from rx are transmitted directly to servos with minimal delay
    - Do direct first, then see if I can do normal?
  - [x] Normal: inputs from rx are monitored by computer and corrected for things such as limits, holding bank/pitch, etc. (look up what airbus normal law does lol)
- [x] Control modes should be toggleable by a switch on tx
- [x] Efficient and fast code so there is minimal delay between input, processing, and output (it shouldn't affect flying and reaction time)
  - [x] Possibly do some multicore code? I know C is very fast but if it's not too hard, it might be useful to split the workload between the two cores. One core solely handles I/O (taking input from rx and actuating servos) and the other core does all of the FBW math, PID, etc.
- [x] Scalability and flexibility (eg. should be able to redefine what pins to connect servos to, number of servos for different purposes, etc.)
  - [x] Add some auto tuning capabilities--namely tuning the midpoint of the PWM input signal and possibly a guide/semi auto tuned PID
    - [x] Try and make use of the flash to store these tuning values
    - For the PID values, maybe make another program to store them in the 2nd to last sector of flash (last sector is PWM tuning currently); eg you can type PID values into the serial port. it should probably be better than forcing people to download and compile source...

Future development ideas:

- [ ] Rudder control? This would be in the form of a yaw damper. I can't currently think of how I would implement this, but this is pretty high on features I want in here. I left a lot of the original groundwork to control a rudder in the code for this exact reason.
- [ ] Motor/throttle control?
- [ ] Autopilot: maybe add a GPS module for pos/alt and make some sort of path/mission planner
  - [ ] Adding on to that, more modes for the autopilot? Something like Ardupilot with auto takeoff, climb, cruise, approach, landing--efficively making it an autonomous drone basically.
  - [ ] Integration with a more powerful chip? Like having another piece of software running on a Pi zero for example (and hopefully not C omg), this might actually be necessary if I want to do some higher level stuff like nested PIDs for gps, LNAV/VNAV so as to not overload the microprocessor. It also makes me look cooler.

Eventually, this readme should have a:

## Download

## Setup

## Features

## Building/Prerequisites
