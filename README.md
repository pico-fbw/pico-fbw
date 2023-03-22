# pico-fbw

A fly-by-wire system for RC airplanes designed to run on the Rasperry Pi Pico microcontroller.

This is still very much a work in progress/alpha project! Feel free to bookmark this and keep up on development, and be sure to let me know of any suggestions you may have. Thanks for stopping by!

## Development Goals

- At least 2, maybe 3 control modes
  - Direct: inputs from rx are transmitted directly to servos with minimal delay
    - Do direct first, then see if I can do normal?
  - Normal: inputs from rx are monitored by computer and corrected for things such as limits, holding bank/pitch, etc. (look up what airbus normal law does lol)
  - Autopilot: very primitive for now if I can, but more of just an auto level mode (there aren't enough sensors to allow pathing yet, maybe add a GPS for pos/alt down the road?)
- Control modes should be toggleable by a switch on tx
- Efficient and fast code so there is minimal delay between input, processing, and output (it shouldn't affect flying and reaction time)
- Scalability and flexibility (eg. should be able to redefine what pins to connect servos to, number of servos for different purposes, etc.)
- Eventual motor control?

I'm keeping the list relatively short for now because this is my first microcontroller project/C project so I don't want to have too much over my head. Plus, I can already see the PID loops being painful enough. Good luck to me :)

Eventually, this readme should have a:

## Download

## Setup

## Features

## Building/Prerequisites
