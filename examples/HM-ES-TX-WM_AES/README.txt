HM-ES-TX-WM
so far only tested with an infrared sensor on a ferrari counter. 
The current implementation of InfraredSignalDetector.c conflicts with the battery measurement.
To get readings i needed to disable the bt.poll calls in AS.cpp.
Not tested with AES

Building
if platformio is setup on your system running "platformio run -t upload" in the current directory should be enough


Used Parts from aliexpress
- arduino nano v3
- cc1101
- line tracking sensor LM393 / tcrt500


