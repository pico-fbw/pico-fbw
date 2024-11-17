# pico-fbw

A cost-effective, intuitive, and reliable remote control autopilot solution built for the future.

## Why (not) pico-fbw?

pico-fbw was created to serve as a cheap and simple entry into the world of electronically controlled model aircraft, whether that be fly-by-wire control assistance or a complete autopilot solution.

The goal of this project is to make RC fly-by-wire and autopilot technology more accessible, mainly through means of cost and ease-of-use.
Keeping this in mind, pico-fbw is not extremely advanced, very feature-dense, or well-established. Other projects, including the likes of [Ardupilot](https://github.com/ArduPilot/ardupilot), [INAV](https://github.com/iNavFlight/inav), [BetaFlight](https://github.com/betaflight/betaflight), and many more serve that purpose well.

That being said, pico-fbw is nonetheless quite a capable and robust flight controller, introducing features such as:

## Features

- Safe and tested; automatic safety checks throughout the software + always-active manual overrides
- Real-world fly-by-wire-like stabilization
- Simple autopilot/autothrottle and accompanying flight planner
- Easily plan flights, modify configuration, diagnose faults, and more, with just a smartphone/tablet/computer
- Good out-of-box performance, with optional automatic tuning to further improve performance
- Utilizes common and low-cost off-the-shelf microcontrollers and sensors
- Easy hardware and software setup
- Extensive documentation to make setup and usage easy
- Versatile; well-supported ability to retrofit into many existing aircraft + their equipment
- Built-in API allows for external control using another device
- Fully open-source
- And more!

## Requirements

- An existing RC aircraft setup
  - This must include (but is not limited to): a fixed-wing aircraft (airplane or flying wing), receiver/transmitter with one spare switch channel, motor controller + battery, etc.
- A microcontroller capable of running pico-fbw
  - See the [supported platforms list]() to determine which is the bet fit for you.
- The necessary sensors (IMU, barometer, GPS)
  - Again, take a look at the [supported sensors list]() to determine which to utilize or purchase.
- A handful of female-to-female and male-to-female jumper wires

If there is enough interest, the pico-fbw project will consider producing dedicated pico-fbw hardware to reduce setup complexity and cost.

## Setup

pico-fbw stands in the middle between your radio receiver and output devices.

It may seem daunting to to modify your existing setup to incorporate this, but rest assured, it is relatively simple and won't take long. A [step-by-step guide](https://github.com/pico-fbw/pico-fbw/wiki/_Setup) has been created that outlines exactly how to wire things up and get going with pico-fbw!

### Download

You can always find the latest stable release of pico-fbw for all supported platforms on the [Releases page](https://github.com/pico-fbw/pico-fbw/releases/latest).

## Building & Contributing

See [the wiki](https://github.com/pico-fbw/pico-fbw/wiki/Contributing) for more details.

## Issues, Feedback, and Features

If you experience any issues, have any ideas for new features, or just have any general feedback about pico-fbw, don't hesitate to reach out! You can submit an issue on the [issues page](https://github.com/pico-fbw/pico-fbw/issues/new)--just please be sure to label your issure accordingly.

If you are a developer looking to improve on pico-fbw's code, feel free to leave an issue or [pull request](https://github.com/pico-fbw/pico-fbw/compare)!

## **DISCLAIMER**

pico-fbw has the ability to control your aircraft. Despite the thorough limitations in place, please exercise extensive caution when operating the aicraft so as not to harm yourself, others, or your aircraft. You, not pico-fbw's authors, are responsible for your own aircraft and any damages caused by it.
