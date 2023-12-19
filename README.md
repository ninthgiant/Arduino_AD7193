# README for Arduino Project: AD7193_v035_commonCode

This project contains code common with the Arduino project HX711_R4_R3_v02 which runs on both the Uno Rev3 and Uno Rev4.

This project is built to control an Arduino UNO R3 only since the AD7193 library is not compatible with the Uno Rev4 Renesas. Most of the code matches that of the HX711 project except for amplifier-specific code.

Project controls an Arduino with an IC2 LCD screen, an Adafruit SD/RTC shield and a load cell connected to the AD7193 amplifier on a PMOD AD5 board.