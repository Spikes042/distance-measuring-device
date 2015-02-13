
# distance-measuring-device

A distance measuring device with an ultrasonic rangefinder and a DE0 FPGA development board running a NIOS II processor


# Design Methodology

The project is to construct a distance measuring device with an ultrasonic rangefinder and a DE0 FPGA development board running a NIOS II processor. The DE0 FPGA development board initiates the ultrasonic rangefinder to send an ultrasonic beam. The rangefinder waits for the beam to return and sends a signal to the DE0 FPGA development board when it receives the beam echo.

The DE0 FPGA development board calculates the distance by using the time taken for the ultrasonic beam to return an echo and it’s running time. The calculated distance is measured in centimetres and millimetres and is displayed on the seven segment display.

The device also saves the maximum and minimum distances measured and a reset functionality which resets the saved distances.

# The Hardware Components

1.	A DE0 FPGA development board
2.	An SRF05 Ultrasonic rangefinder
3.	A USB cable
4.	A breadboard
5.	A 1k Ohms resistor
6.	A set of Wires

# Development Software
Altera NIOS II software on Quartus II version 13.0 (Web Edition)
