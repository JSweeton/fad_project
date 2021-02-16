| Supported Targets | ESP32 |
| ----------------- | ----- |

# Description
This program is the bluetooth version of the FAD Digital Masker. This program utilizes AVCRP and A2DP to 
communicate control commands between an ESP32 and a bluetooth device and to transmit audio to such a device.

It also uses the Espressif timer API for ADC timing for an input microphone and the FreeRTOS operating system for task
management.

Flash and install this to the ESP32 using the following command (following setup of the ESP-IDF):
```
idf.py -p [port_name] flash monitor
```
This will build the program and install it onto the ESP32. 

# High Level Overview
## Startup
At program startup, the ESP32 does some initial tasks.
- Initialize and Enable Bluetooth Controller
- Initialize and Enable Bluedroid (Bluetooth-framework)
- Start task manager
- Add stack initializer task to task manager
- Setup Secure Simple Pairing

Then, the event BT_APP_EVT_STACK_UP triggers in the bt_av_hdl_stack_evt function. This gives the device 
a name, registers callback functions for different libraries, initializes AVRCP in controller mode, and
starts discovery for Bluetooth's GAP protocol (Sets to Connectable and Discoverable). Also, a 'heartbeat'
timer is created, which will send a BT_APP_HEART_BEAT_EVT event to the task manager every 10 seconds.

## Runtime
During runtime, the program operates on events. The lower level Bluetooth libraries send events with paramaters
to the callback functions that were initialized during startup (i.e. the programmer creates and names these 
functions).

All events are in practice routed through the state machine handler function (bt_app_av_sm_hdlr), which parses 
events from A2DP, AVRCP, and the Heartbeat timer, and interprets those events in relation to the current state
of the device (e.g. Discovering, Discovered, Unconnected, Connecting, Connected, Disconnecting).

## General Flow
The program first begins GAP discovery to find information about all local devices. Based on their respone, the 
program can choose a device to connect to. Once our device connects to a peer A2DP-capable device (state=conneceted), 
our device begins AVCRP communication with the device to prepare it for audio transmission.

