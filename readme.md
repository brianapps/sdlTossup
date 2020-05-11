# SDL Tossup simulator

The project is a cross-platform recreation of the Game and Watch Toss Up simulator using SDL. The code is based on the Win32 Toss Up simulator by Brian Apps (http://brianapps.net/tossup).

## Building

The project uses CMake and builds SDL from source rather than picking up preinstall binaries.


### Windows - Visual Studio

 1. Download the SDL source (https://www.libsdl.org/release/SDL2-2.0.12.zip) and unzip this so it is under `external\SDL2-2.0.12`.
 2. Ensure Visual Studio 2019 has the CMake option has been installed.
 3. Open this source folder and CMake should configure the project it can be built in the normal way.

The SDL source should we placed under `external\SDL2-2.0.12`.

### Linux/Mac

In short the process is.
 1. Fetch the source code for this project.
 1. Download the SDL source (https://www.libsdl.org/release/SDL2-2.0.12.zip) and unzip this so it is under `external\SDL2-2.0.12`.
 1. Run CMake in an out of source directory and build.

This can be achieved as follows:

```
git clone ....
cd sdlTossup
mkdir external
cd external
curl https://www.libsdl.org/release/SDL2-2.0.12.tar.gz | tar zxv
cd ..
mkdir build
cd build
cmake ..
cmake --build . -- -j8
```

### Raspberry Pi

The project has been tested using OpenGLES on various devices. It may work under X11 but this hasn't been tested. The recommended approach to run this program on a Raspbian Lite image (Buster at the time of writing).

Install the libraries. WiringPi can be omitted if you do not intend to use the GPIO ports to control the game.

```
sudo apt-get install cmake libasound2-dev libudev-dev wiringpi
```

Follow the same steps as the Linux build.

```
git clone ....
cd sdlTossup
mkdir external
cd external
curl https://www.libsdl.org/release/SDL2-2.0.12.tar.gz | tar zxv
cd ..
mkdir build
cd build
cmake ..
cmake --build . -- -j4
```


### Cross compile for Raspberry Pi

It is possible to cross compile the project. The steps below use a prebuilt cross compiler from 


1. Fix up the SDL CMake scripts. At the time of writing there's a little issue with the CMake scripts that prevent the correct configuration for cross-compile build. To fix this edit `SDL2-2.0.12/cmake/sdlchecks.cmake` and change the five places where `/opt/vc` is specified with `{CMAKE_SYSROOT}/opt/vc`. These are all in the `CheckRPI` macro.
1. Update the sysroot on the local machine. It appears the `UpdateSysroot` tool no longer ships with the cross compiler so this can be done manually by taring it up:
   ```
   tar -chf sysroot.tar --hard-dereference  /etc/ld.so.conf* /opt/vc /lib /usr/include /usr/lib /usr/include /usr/local/lib /usr/local/
   ```
   (Ignore the _removing leading '/'_ warning and and 'ssl/private permission denied error). Then transferring the tar file over and unpacking it to a suitable directory (e.g. `C:\SysGCC\rasbperry\newsysroot`).

   **Note:** the above might not work if `ld.so.conf` uses includes and the workaround is to create a single file that has the content of all the includes. e.g. something like `(for %i in (ld.so.config.d\*) do @type %i) > ld.so.conf`.
1. Create a CMake toolchain file, name `RpiToolchain.cmake`
   ```
   set(CMAKE_SYSTEM_NAME Linux)
   set(CMAKE_SYSTEM_PROCESSOR arm)

   set(CMAKE_SYSROOT /Apps/SysGCC/newsysroot)

   set(tools /Apps/SysGCC/raspberry)
   set(CMAKE_C_COMPILER /Apps/SysGCC/raspberry/bin/arm-linux-gnueabihf-gcc.exe)
   set(CMAKE_CXX_COMPILER /Apps/SysGCC/raspberry/bin/arm-linux-gnueabihf-g++.exe)

   set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
   set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
   set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
   set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
   ```
4. Build with something like
   ```
   md rpi
   cd rpi
   cmake -G Ninja -CMAKE_TOOLCHAIN_FILE=..\RpiToolchain.cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build .
   ```

## Running

To run the program execute `SDLTossup`. While running the following keys can be used:

| Key | Function        |
| --- | --------------- |
| Q   | Move arms left  |
| P   | Move arms right |
| A   | Start Game A    |
| B   | Start Game B    |
| T   | Enter time mode |
| X   | Exit            |

The Raspberry Pi version allows key functions to be read directly from the GPIO pins. See the Raspberry Pi GPIO section below.

### Command line options

```
SDLTossup [-f] [-d <display_index>] [-s <subsamples>] [-lcd <hexcolour>] [-back <hexcolour>] [<width> <height>]
```

Where

| Option     | Description                                                                          |
| ---------- | ------------------------------------------------------------------------------------ |
| -f         | run game in fullscreen mode.                                                         |
| -d         | display game on the monitor given by `<display_index>.`                              |
| -s         | size `<subsamples>` by `<subsamples>` grid used for anti-aliasing. Can be 1 to 15.   |
| -lcd       | the colour in as an BBGGRR hex string for the LCD elements. Defaults to 424242.      |
| -back      | the colour in as an BBGGRR hex string for the screen background. Defaults to 708080. |
| -info      | Show display and audio info and then exit.                                           |
| `<width>`  | width of the game texture.                                                           |
| `<height>` | height of the game texture.                                                          |

# Raspberry PI GPIO

The Raspberry Pi version allows the game functions to be read directly from the GPIO pins. This allows custom controller to be developed that don't need to emulate a keyboard.

The [Wiring Pi](http://wiringpi.com/) library is used to read the GPIO pins and should be installed before building the program.

6 pins are used for input and they all are configured to use the Pi's internal pull down resistors. The each (normally open) switch should therefore be wired to 3v3 (pin 1) via a current limiting resistor (say 270 ohms). The pins numbering follows the BCM scheme (see https://pinout.xyz for details).

| GPIO Number | Physical Pin Number | id  | Function   |
| ----------- | ------------------- | --- | ---------- |
| BCM 17      | 11                  | L   | Move left  |
| BCM 27      | 13                  | R   | Move right |
| BCM 22      | 15                  | A   | Game A     |
| BCM 23      | 16                  | B   | Game B     |
| BCM 24      | 18                  | T   | Time       |
| BCM 25      | 22                  | X   | Exit       |

ASCII art of the connection I use on the pi zero is as follows. (Note `<`, `>` and `0` refer to the pins used to get audio output from the pi zero, see https://learn.adafruit.com/adding-basic-audio-ouput-to-raspberry-pi-zero.)

```
02 04 06 08 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40
.  .  .  .  .  <  .  B  T  .  X  .  .  .  .  .  0  .  .  .
+  .  .  .  .  L  R  A  .  .  .  .  .  .  .  .  >  .  .  .
01 03 05 07 09 11 13 15 17 19 21 23 25 27 29 31 33 35 37 39
```
