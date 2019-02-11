# estacao_meteorologica
An Arduino-based weather station using an OLED display and multiple sensors

Information shown and sensors used:
 * - time and date (DS1307)
 * - temperature (DHT22)
 * - barometric pressure, altitude, relative humidity (BMP05)
 * - weather forecast, based on pressure and temperature data variance collected over the last few hours
 
 Everything is shown on a generic OLED 0.96" display.
 
 These are the pinouts and where each sensor should be connected to the Arduino Nano.
 
_DHT22_: 
 *    OUT = D2
 *    Vcc = 5V
 *    GND = GND
 
 _BMP05_: 
 *    Vin = 3.3v
 *    GND = GND
 *    SCL = A5
 *    SDA = A4
 
 
 _DS1307_:
 *    Vcc = 5V
 *    GND = GND
 *    SCL = A5
 *    SDA = A4
     
 _OLED Display_:
 *    GND = GND
 *    Vcc = 5V
 *    SCL = D13
 *    SDA = D11
 *    RST = D8
 *    D/C = D9
 
 Enjoy!
 
