# pico-fbw

A fly-by-wire system designed for RC airplanes, for the Rasperry Pi Pico microcontroller.

## **DISCLAIMER**

This system is able to control most aspects of your aircraft. Even though there are limitations in place, *please* exercise extra caution when operating the aicraft so as not to harm yourself, others, or your aircraft! I have gone through extensive testing to make the system as safe as possible, but be aware that at the end of the day *you* are responsible for your own aircraft and any damages caused by it.

Now, with that out of the way, let's get into the features of the system!

## Features

- Typical features of a real-world fly-by-wire aircraft, such as:
  - Limiting of pitch and bank angles
  - Automatic in-flight stabilization using an onboard sensor unit
  - Holding of requested angles
  - Auto-coordinated turns using a yaw damper
- Implementation of direct mode (user inputs are transmitted directly to flight controls)
  - It can be easily activated with the use of a switch on the transmitter
- Wi-Fly allows you to easily upload flight plans over Wi-Fi to be automatically flown
- Engineered and tested for safety all-around  
- Heavy optimization for the Pico's hardware leads to minimal input lag on the controls
- Extensive documentation to make setup and usage easy
- Very cheap to get started; an easy entrypoint to automated RC flight
- Support for both standard and Pico W models
- Flexibility; the system can easily be reconfigured through a configuration file to suit your and your aircraft's needs
- Fully open-source codebase with commented code

## Materials

You will need a few materials in addition to your current RC plane setup. They are as follows:

- Raspberry Pi Pico microcontroller (with data-capable micro-USB cable to flash the program)
- BNO055 sensor breakout
- ~7 female-to-female jumper wires
- ~10 male-to-female jumper wires
- A switch on your transmitter and corresponding channel on your receiver

You can find a guide to sourcing these parts as well as my tested/preferred sources on our [materials wiki page](https://github.com/MylesAndMore/pico-fbw/wiki/Materials).

## Download

You can always find the latest stable release of the software on our releases page [here](https://github.com/MylesAndMore/pico-fbw/releases/latest).

This binary is pre-built so you can drag and drop it onto your Pico as-is, but be aware it only ships with recomended configuration values. These values may need to be altered in your case as different aircraft behave very differently, and the values that work for me may not work for you. So please, be careful, and test thoroughly! Do not hesitate to reconfigure if you need to.

## Setup

The Pico acts like a person in the middle. Instead of your servos, ESC, and such being wired up directly to your reciever, instead, your reciever is wired up to the Pico so it can recieve and compute those inputs, and your output devices (like servos) are also wired up to the Pico so it can control them.

It may seem daunting to to modify your existing setup to incorporate this, but rest assured, it is relatively simple and will not take you long. I've created a guide for you to check out [here](https://github.com/MylesAndMore/pico-fbw/wiki/_Setup) that outlines exactly how to wire things up and get going with the project!

## Building & Configuring

You can find all of the information about building and configuring the project on our wiki page [here](https://github.com/MylesAndMore/pico-fbw/wiki/_Building-&-Configuring).

## Issues, Feedback, and Features

If you experience any issues, have any ideas for new features, or just any general feedback about the project in general, don't hesitate to reach out! You can submit an issue on our [issues page](https://github.com/MylesAndMore/pico-fbw/issues/new)--just please be sure to label your issure accordingly. And if you are a developer looking to suggest or improve on our code, feel free to leave us an issue or [pull request](https://github.com/MylesAndMore/pico-fbw/compare)!

Thanks for checking out my project, hope you enjoy! :)
