# pico-fbw

A fly-by-wire system for RC airplanes designed to run on the Rasperry Pi Pico microcontroller.

This is still very much a work in progress/alpha project! Feel free to bookmark this and keep up on development, and be sure to let me know of any suggestions you may have. Thanks for stopping by!

## Development Goals

[] At least 2, maybe 3 control modes
  [x] Direct: inputs from rx are transmitted directly to servos with minimal delay
    - Do direct first, then see if I can do normal?
  [] Normal: inputs from rx are monitored by computer and corrected for things such as limits, holding bank/pitch, etc. (look up what airbus normal law does lol)
  [] Autopilot: very primitive for now if I can, but more of just an auto level mode (there aren't enough sensors to allow pathing yet, maybe add a GPS for pos/alt down the road?)
[] Control modes should be toggleable by a switch on tx
[] Efficient and fast code so there is minimal delay between input, processing, and output (it shouldn't affect flying and reaction time)
  - Possibly do some multicore code? I know C is very fast but if it's not too hard, it might be useful to split the workload between the two cores. One core solely handles I/O (taking input from rx and actuating servos) and the other core does all of the FBW math, PID, etc.
[x] Scalability and flexibility (eg. should be able to redefine what pins to connect servos to, number of servos for different purposes, etc.)
  [] Add some auto tuning capabilities--namely tuning the midpoint of the PWM input signal and possibly a guide/semi auto tuned PID
    [x] Try and make use of the flash to store these tuning values
    - For the PID values, maybe make another program to store them in the 2nd to last sector of flash (last sector is PWM tuning currently); eg you can type PID values into the serial port. it should probably be better than forcing people to download and compile source...
- Eventual motor control?

I'm keeping the list relatively short for now because this is my first microcontroller project/C project so I don't want to have too much over my head. Plus, I can already see the PID loops being painful enough. Good luck to me :)

Eventually, this readme should have a:

## Download

## Setup

## Features

## Building/Prerequisites
