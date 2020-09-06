| Supported Targets | ESP32 |
| ----------------- | ----- |


#FAD Project Program

##Deliverable:
Provide a device that can:
- Detect a user's voice
- Manipulate voice audio into white noise or pure frequency
- Return this manipulated audio to the user's ear to mask their own voice

##Strategy:
This program utilizes the **FreeRTOS** ([reference](https://www.freertos.org/Documentation/RTOS_book.html)) operating system provided for the ESP32, as well as the **A/D** and **D/A** API's for the ESP32. 
- A/D converter will place user audio input into an input buffer
- Core 0 will do basic signal processing and analysis on input buffer data
- Based on input data, core 0 will create a masking signal
- D/A converter will transform digital masking signal into audio for user's ear

##Team Members:
- Larry Vega
- Michael Jenkins
- Corey Bean

**README Guide:**
```
Code Snippit: Surround with ```
Heading: Begin with # (number of # determines heading category)
Bullet Points: Begin points with -
Bold: surround with ** (ex. **bold**)

```