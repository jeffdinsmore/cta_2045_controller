# CTA2045 UCM C++ Library v.1.00
Copyright (c) 2016, Electric Power Research Institute (EPRI)
All rights reserved.

## License
This software is released by EPRI under the BSD 3 Clause license. See LICENSE
for details.

## Overview
This software implements the CTA2045 UCM protocol as defined by the Consumer
Electronics Association in the R7.8 Modular Communication Interface for Energy
Management subcommittee.

This project includes these components:

* **cea2045**: Core CTA2045 Library
* **controller**: CTA-2045 controller application and command scheduler
* **third_party**: Bundled external dependencies used by the controller
* **test**: Unit test sources for the core library

The library is written in C++ and uses some features of C++11.

## Dependencies
This project was developed and tested on Ubuntu 16.04, and is running in production
on OpenWRT Linux for EPRI's UCM (MIPS) as well as Raspbian and OpenWRT on mulitple
revisions of Raspberry pi (ARM).

This project does not rely on any 3rd party libraries, but requires the following
 build dependencies:
* cmake, make, g++

These dependencies can bin installed on Ubuntu with the following command:
```
sudo apt-get install g++ cmake make
```

## Build instructions
Run the following commands depending on the type of build required. The commands
assume starting from the root of the source tree.

Building the library generates `libcea2045.so`. Building the controller generates
an executable named `cta2045_controller`.

* **Debug**
  ```
  mkdir -p build/debug
  cd build/debug
  cmake -DCMAKE_BUILD_TYPE=Debug ../../
  make
  ```

* **Debug with controller**
  ```
  mkdir -p build/debug
  cd build/debug
  cmake -DCMAKE_BUILD_TYPE=Debug -DCONTROLLER=ON ../../
  make
  ```

* **Release** (for production builds)
  ```
  mkdir -p build/release
  cd build/release
  cmake -DCMAKE_BUILD_TYPE=Release -DCONTROLLER=ON ../../
  make
  ```

## Notes on the testcea2045 program
The test program is a set of unit tests. To run the program, navigate to the 
build/release or build/debug folder and execute:
  ```
  ./testcea2045
  ```

If the tests are successful, the following should be output:
  ```
  ===============================================================================
  All tests passed (164 assertions in 11 test cases)
  ```

The following options are of special interest:
-? or -h displays the help information
-l lists all the tests
-s runs and provides a verbose results output of each test

## Notes on using the controller
The controller communicates with an SGD device through a serial
port. The best approach is to run it with the EPRI CTA-2045 simulator and test 
cables. This section describes running the controller on
a Raspberry Pi single board computer combined with the CTA-2045 simulator 
application.

1) Load the files on a Raspberry Pi 2 or 3.
2) Load the dependencies as described in the Dependencies section above.
3) Build the application per the instructions in the Build instructions section. 
   Use the Release instructions.
4) Run `make schedule` to create a schedule, then `make run`.

The controller attempts to open `/dev/ttyUSB0`. Adjust the serial-port setting
in `controller/main.cpp` when necessary.

On startup, the program will send a few commands to the SGD to determine
what features are supported. Next, the program runs in a loop waiting for user
input to send commands to an SGD. The commands are a single character followed by 
enter. The supported commands are as follows:
c:	basic Critical Peak Event with param = 5
e:	basic End Shed Event with param = 0
g:	basic Grid Emergency Event with param = 5
l:	basic Load Up Event with param = 5
n:	basic Next Relative Price Event with param = 153 (approx 4x)
o:	basic Outside CommConnection Status with param = 1
p:	basic Power Level with param = 63 (approx 50%)
q:	shutdown
r:	basic Present Relative Price with param = 101 (approx 2x)
s:	basic Shed Event with param = 5
C:	intermediate Get Commodity
O:	intermediate Get TemperatureOffset
S:	intermediate Get SetPoint
T:	intermediate Get Present Temperature

## Creating a UCM with the library
The best way to learn how to use the library is to look at `controller`, but here
are a few tips.

The library uses callbacks to notify user code when responses and unsolicited
messages are received from the SGD. The programmer passes a `IUCM`
implementation (which contains the callbacks) to the `DeviceFactory::createUCM()`
function when creating a device.

### Communicating with an SGD
The device won't start processing messages until the device's `start` function
is called. To stop communication, call `shutDown` on the device. Calling the 
`start` creates a thread which processes incoming and outgoing messages from the 
serial port.

Functions called on the device are non-blocking: messages are queued and processed
in order; each function call returns a future. To force your program to block 
until the command is complete, call `get()` on the returned future.

Each function call returns a `ResponseCode` object which indicates the success
or failure of the function call.

### Additional Testing Notes

