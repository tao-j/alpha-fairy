# Firmware Architecture

I've written a bunch of libraries when appropriate, things that can be used independantly by other applications. This also makes it easy to do tests.

 * the PTP-over-TCP/IP stuff and the Sony camera object is in one library called PtpIpCamera
    * in theory it is extendable to Canon and Nikon
 * infrared command sending
 * low level Wi-Fi initialization and new client detection
 * extended image file decoding for LCD display
 * image sprite caching linked list
 * serial port command line handler (I wrote my own custom one instead of borrowing one)
 * serial port objects that can be enabled or disabled individually, like log levels, except not levels, but sources

3rd party libraries are used for the M5StickC hardware and Wi-Fi.

Of course the main application involves a complex interaction between all of these libraries and the application itself.

The ESP32 Wi-Fi stuff runs on its own RTOS thread, as ESP intended, and takes advantage of multi-core. Everything else is on another thread, the one that runs Arduino's `loop()` function. The Wi-Fi API calls are synchronous (I did try asynchronous and it didn't add much benefit). The various modules I wrote that has asynchronous elements offer a `poll()` or `task()` function that can be called during wait-loops.

The menu system involves a table with image file paths and function pointers. Navigating a menu is basically indexing that table when the side-button is pressed, and calling the function when the big-button is pressed. Adding additional menu items, or re-organizing the items, is super easy. There are multiple tables and menus can be nested.

For configuring setting items, there's also a table that contains a pointer to the item's memory, the text that is shown, and the properties of that item (upper and lower limit, step size, displayed unit, etc). Adding to or re-organizing this table is super easy.

The code that handles the status bar, which reads the PMIC data (battery voltage and etc) is in a polling function that runs only periodically. The I2C transactions are slow and cost battery power.

The major menu items uses full screen sized PNG files, and is only refreshed when required, which saves on some power and lag since it's slow to read from external flash and then updating the entire screen. Text elements are drawn quickly directly to only a small region of the screen. Smaller icons, such as the status bar, or the spinning clock icon, are cached into sprites to avoid the read from external flash.

## User Interaction

It's fun trying to create a good user experience with only two buttons and a screen, luckily there's also an IMU sensor too. In general, the side-button means "next", the big-button means "activate", and the power button only turns the device on and off.

The M5StickC library provides an implementation of the AHRS filter to calculate angles from accelerometer and gyroscope data. The output isn't very good but is good enough for me to not bother re-writing the filter.

For focus pulling, the user tilts the device left or right to determine both the direction and speed of focus change.

To adjust values in the configuration screens, tilting left means the user wants the value to go down, tilting right means the user wants the value to go up.

To change the delay for remote shutter (which only offers 3 options), the user spins the whole device around to change the delay. This is currently slightly buggy because the ARHS output does not handle upside-down at all, a very janky workaround is used to handle a full spin.

To set the shutter speed and ISO for the dual-shutter, the setting is actually obtained from the camera itself when the user pressed the big button. As in, the camera itself is the user input. The trigger to activate dual-shutter is also constantly monitoring the camera's focus status, so when the user presses the shutter button on the camera half-way down, the dual-shutter code executes.

My original implementation had one long chain of menu items, and the user could tilt left to navigate backwards. But after adding more features, I decided to go with nested menus instead of just one flat menu.

## Resource Usage

The built process (on August 7 2022) reports that

    Sketch uses 851353 bytes (64%) of program storage space. Maximum is 1310720 bytes.
    Global variables use 66608 bytes (20%) of dynamic memory, leaving 261072 bytes for local variables. Maximum is 327680 bytes.

The local variable memory is further used for image sprites. It actually does not have enough memory to store every single sprite that I can come up with, so the sprite manager is constantly being unloaded.

The PNG files occupy a bit less than 300kb total (I am using PngOptimizer to make sure they are totally compressed). There should be 4MB of flash memory available for files like this.

This project is written in C++, and already occupying 64% of total available memory. The M5StickC does support MicroPython, and I did initially attempt to write this whole project using MicroPython. That failed, the python code actually ran fine on my PC, but threw a out-of-memory exception as soon as it tried the first `import` statement on ESP32. The big problem is that the MicroPython port for M5StickC is horribly inefficient. [I have written an entire rant about it.](M5StickC-MicroPython-Sucks-Rant.md)