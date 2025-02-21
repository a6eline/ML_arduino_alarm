# Alarm Clock with ML Voice Recognition
An alarm clock which has to be turned off remotely elsewhere using esp_now one way communication to split system into 2 devices. The purpose of this is to ensure that users get out of bed to turn it off instead of snoozing and going back to bed. This can take in ESP and Arduino BLE boards. 

Status: Ongoing 
> ML Voice Recognition Model is made by @abbie_bola

## Board Prototype Photos
The prototype uses an Arduino nano board since other ones are not available on Wowki.com all nano boards have the similar pinouts anyways. All jumpwires are pinned in the same place and SDA/SDL are in A4/A5 respectively - the [Nano ESP32](https://docs.arduino.cc/resources/pinouts/ABX00083-full-pinout.pdf) and [Nano 33 BLE Sense Rev2]([https://docs.arduino.cc/resources/pinouts/ABX00083-full-pinout.pdf](https://docs.arduino.cc/resources/pinouts/ABX00031-full-pinout.pdf)) for reference. There is no mic module in wowki.com unfortunately so was not able to show that - photos will be added soon! 

### Alarm Clock Board
![alarm_clock_pinout](https://github.com/user-attachments/assets/84e0cc8f-e5a3-41ba-81e9-2eeefc2f4cef)
![button_pinout](https://github.com/user-attachments/assets/b55ec6d9-76c8-48b1-8c9a-9a881498567f)

## Materials Used
- [Arduino NANO ESP32 Board](https://docs.arduino.cc/hardware/nano-esp32/)
- LCD1602 + Potentiometer
- [DS3231 RTC Module](https://www.amazon.co.uk/AZDelivery-RTC-Battery-included-Raspberry/dp/B01M2B7HQB/ref=sxin_15_pa_sp_search_thematic_sspa?cv_ct_cx=ds3231%2Breal%2Btime%2Bclock%2Bmodule&sbo=RZvfv%2F%2FHxDF%2BO5021pAnSA%3D%3D&sr=1-2-ad3222ed-9545-4dc8-8dd8-6b2cb5278509-spons&sp_csd=d2lkZ2V0TmFtZT1zcF9zZWFyY2hfdGhlbWF0aWM)
- INMP441 Mic Module (must be soldered on) + Passive Buzzer + Button

## [Project Documentations & Log](https://docs.google.com/document/d/1WiQw86Ue8yddEHVPHRZVkMrpHQOxgGgfX3WtC2Yl9dU/edit?usp=sharing)
Project documentation includes a time log, Git Branch Tracker, 

## Libraries Used
- [RTC_lib.h](https://github.com/adafruit/RTClib) - for the DS3231
- [Wire.h](https://github.com/arduino/ArduinoCore-avr/blob/master/libraries/Wire/src/Wire.cpp) - for I2C communication 
- [LiquidCrystal.h](https://github.com/arduino-libraries/LiquidCrystal) - for the LCD1602
- [esp_now.h](https://github.com/espressif/esp-idf/blob/master/components/esp_wifi/include/esp_now.h) - for wireless ESP communication
- [WiFi.h](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFi.cpp) - for WiFi communication

## To Add Soon
- FSM details + Alarm System
- How to use DS3231
- How to setup ESP wireless communication
- ML + ESP 
