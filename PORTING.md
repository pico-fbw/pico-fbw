# Porting

If you're interested in porting pico-fbw to a new platform, read on!

- Before you get started, **consider the practical requirements.**
While it doesn't require loads of processing power, pico-fbw was designed with newer, ARM/RISCV-based microcontrollers in mind.
Consider: does your platform provide enough power to support running moderately heavy calcuations (such as the fusion algorithm) at a fast rate?
- Obviously, **your platform must have a C compiler.**
GCC is preferred and all current ports use it, but other toolchains like Clang might work with some modification.
- **We've created an "example" directory to ease in porting.**
Start by duplicating the directory and renaming it to the name of your platform.
- **Take a look at all the files in the folder.**
They contain numerous hardware/platform-specific functions that must be completed in order for pico-fbw to work on your platform.
All functions contain baseline documentation in their respective header (.h) files in `platform/`, and
some additional comments in the source (.c) files as needed.
  - **Don't forget to set up the build system!** pico-fbw provides some functions that you should define in `platform/YOUR_PLATFORM/resources/YOUR_PLATFORM.cmake`.
  These should set up CMake to build for whatever platform you've ported to. More information can be found in the .cmake file.
- **Just because something compiles doesn't mean it works!** Test thoroughly, first on the ground and later in flight.
- **Feel free to look around for inspiration!** pico-fbw already works on other platforms.
If you get stuck, take a look at them to see how they implemented the same functions.

Once you're done...

- **Write some documentation!** A baseline would be a simple README for the platform detailing the required prerequisites to build for your platform.
  - If you're feeling up for it, you could **modify the `build.py` script** to automatically fetch these prerequisites and build.
  But it's not required!
- **Submit a pull request!** We'd love to see your hard work, and we're sure there are others out there with your platform that would benefit from a pico-fbw port.
Open source is pretty cool when it works!

Thanks for your interest, and best of luck in your port! We can't wait to see what you come up with :)
