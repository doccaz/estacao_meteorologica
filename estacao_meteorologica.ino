/********
 * Estacao meteorologica
 * - hora (DS1307)
 * - temperatura (DHT22)
 * - pressão barométrica, altitude, umidade relativa (BMP05)
 * - previsao do tempo, baseado nos dados de pressão e temperatura
 * - display OLED 1"
 * 
 *  Conexões (Arduino Nano):
 *  
 *  DHT22: 
 *    OUT = D2
 *    Vcc = 5V
 *    GND = GND
 *    
 *  BMP05: 
 *    Vin = 3.3v
 *    GND = GND
 *    SCL = A5
 *    SDA = A4
 *    
 *  DS1307:
 *    Vcc = 5V
 *    GND = GND
 *    SCL = A5
 *    SDA = A4
 *    
 *  Display OLED:
 *    GND = GND
 *    Vcc = 5V
 *    SCL = D13
 *    SDA = D11
 *    RST = D8
 *    D/C = D9
 *    
 *****/

#include <Wire.h>
#include "RTClib.h"
#include <DHT.h>
#include <U8glib.h>
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;

#define OLED_RESET 8
#define OLED_DC 9
#define OLED_SDA 11
#define OLED_SCL 13
U8GLIB_SSD1306_128X64 u8g(OLED_SCL, OLED_DC, OLED_RESET);

RTC_DS1307 rtc;
//RTC_Millis rtc;
DateTime now;

// variaveis para previsão de tempo
#define MILS_IN_MIN  60000
int minuteCount = 0;
double pressureSamples[9][6];
double pressureAvg[9];
double dP_dt;
unsigned long startTime;

const char *weather[] = {
  "tempo est" "\xe1" "vel", "previs" "\xe3" "o: SOL", "previs" "\xe3" "o: NUBLADO", "tempo inst" "\xe1" "avel", "previs" "\xe3" "o: CHUVA", "coletando dados"
};
int forecast = 5;

String temp1 = "";
String temp2 = "";
String pressure = "";
String alt = "";
String hum = "";

void setup() {
  // terminal serial para debug
  Serial.begin(9600);

  // inicializa comunicacao I2C
  Wire.begin();

  // inicializa os sensores
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Nao foi possivel achar o sensor BMP180!");
    while (1) {}
  }
  // inicializa RTC
  rtc.begin();
  //rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(2016, 1, 31, 00, 18, 00));

  // timer para previsao
  startTime =  -1;
}

void loop() {
  // realiza uma leitura do sensor
  temp1 = bmp.readTemperature();
  temp1.remove(temp1.length() - 1);

  alt = bmp.readAltitude();
  alt.remove(alt.length() - 1);

  pressure = bmp.readSealevelPressure(alt.toFloat());

  float h = dht.readHumidity();
  hum = (String) h;
  hum.remove(hum.length() - 1);

  float t = dht.readTemperature();
  temp2 = (String) t;
  temp2.remove(temp2.length() - 1);

  // atualiza hora
  now = rtc.now();

  // atualiza a tela
  u8g.firstPage();
  do {
    draw();
  } while ( u8g.nextPage() );

  // calcula a previsao atual e reseta o timer geral
  if (IsTimeout())
  {
    Serial.println("Iniciando cálculo de previsão com pressão = " + pressure);
    forecast = calculateForecast((double) pressure.toFloat());
    startTime = millis();
    Serial.println("nova previsao:" + (String) weather[forecast]);
  }
  delay(1000);

}

void draw()
{
  // frame
  u8g.drawRFrame(1, 10, 127, 42, 1);

  // cabecalho
  u8g.setFont(u8g_font_fixed_v0);
  u8g.setFontPosTop();
  String tm = getTime() + "     " + getDate();
  u8g.drawStr(1, 1, tm.c_str());

  // rodapé (mesma fonte)
  String t = temp1 + "\xb0" "C" + "/" + temp2 + "\xb0" "C";
  u8g.drawStr(1, 55, t.c_str());
  u8g.drawStr(128 - u8g.getStrWidth(getDay().c_str()), 53, getDay().c_str());

  // area de dados
  u8g.setFont(u8g_font_tpssb);
  u8g.setFontPosTop();

  String px = (String) (pressure.toFloat() / 133.32239);
  px.remove(px.length() - 1);
  String p = alt + "m " + px + "mmHg";;

  u8g.drawStr(3, 12, p.c_str());

  // ponto de orvalho: " orv " + (String) dewPointFast(((temp1.toFloat() + temp2.toFloat()) / 2), hum.toFloat()) + "\xb0""C";
  String h = hum + "%rh" + " real " + heatIndex(((temp1.toFloat() + temp2.toFloat()) / 2), hum.toFloat());
  u8g.drawStr(3, 24, h.c_str());

  String f = weather[forecast];
  u8g.drawStr((128 - u8g.getStrWidth(weather[forecast])) / 2, 36, f.c_str());
}

// indice de calor
// referência: https://en.wikipedia.org/wiki/Heat_index
String heatIndex(double tempC, double humidity)
{
  String ret = "";
  double c1 = -42.38, c2 = 2.049, c3 = 10.14, c4 = -0.2248, c5 = -6.838e-3, c6 = -5.482e-2, c7 = 1.228e-3, c8 = 8.528e-4, c9 = -1.99e-6  ;
  double T = ((tempC * 9) / 5) + 32;
  double R = humidity;

  double A = (( c5 * T) + c2) * T + c1;
  double B = ((c7 * T) + c4) * T + c3;
  double C = ((c9 * T) + c8) * T + c6;

  double rv = (C * R + B) * R + A;

  if (rv >= 130)
    ret = "calor extremo";
  else if ((rv >= 120) && (rv <= 105))
    ret = "muito quente";
  else if ((rv >= 90) && (rv <= 104))
    ret = "bem quente";
  else if ((rv >= 80) && (rv <= 89))
    ret = "quente";
  else if ((rv <= 88) && (rv <= 70))
    ret = "morno";
  else ret = "normal";

  double rvC = (((rv - 32) * 5) / 9);
  return (String) rvC + "\xb0""C ";

}


// Retorna string formatada de hora
String getTime()
{
  String timeStr = (String) now.hour() + ":";
  if (now.minute() < 10)
  {
    timeStr = timeStr + "0";
  }
  timeStr = timeStr + (String) now.minute() + ":";
  if (now.second() < 10)
  {
    timeStr = timeStr + "0";
  }
  timeStr = timeStr + (String) now.second();

  return timeStr;
}

// Retorna string formatada de data
String getDate() {

  String dateStr = (String) now.day() + "/" + (String) now.month() + "/" + (String) now.year();
  return dateStr;
}

// Retorna string formatada de dia da semana
String getDay() {
  String dayStr = "";
  switch (now.dayOfTheWeek()) {
    case 0:
      dayStr = "Domingo";
      break;
    case 1:
      dayStr = "Segunda";
      break;
    case 2:
      dayStr = "Ter" "\xe7" "a";
      break;
    case 3:
      dayStr = "Quarta";
      break;
    case 4:
      dayStr = "Quinta";
      break;
    case 5:
      dayStr = "Sexta";
      break;
    case 6:
      dayStr = "S" "\xe1" "bado";
      break;
  }

  return dayStr;
}

// Ponto de orvalho
// referência: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

// rotina "emprestada" do projeto http://goo.gl/l7tdPa
int calculateForecast(double pressure) {
  //From 0 to 5 min.
  if (minuteCount <= 5) {
    pressureSamples[0][minuteCount] = pressure;
  }
  //From 30 to 35 min.
  else if ((minuteCount >= 30) && (minuteCount <= 35)) {
    pressureSamples[1][minuteCount - 30] = pressure;
  }
  //From 60 to 65 min.
  else if ((minuteCount >= 60) && (minuteCount <= 65)) {
    pressureSamples[2][minuteCount - 60] = pressure;
  }
  //From 90 to 95 min.
  else if ((minuteCount >= 90) && (minuteCount <= 95)) {
    pressureSamples[3][minuteCount - 90] = pressure;
  }
  //From 120 to 125 min.
  else if ((minuteCount >= 120) && (minuteCount <= 125)) {
    pressureSamples[4][minuteCount - 120] = pressure;
  }
  //From 150 to 155 min.
  else if ((minuteCount >= 150) && (minuteCount <= 155)) {
    pressureSamples[5][minuteCount - 150] = pressure;
  }
  //From 180 to 185 min.
  else if ((minuteCount >= 180) && (minuteCount <= 185)) {
    pressureSamples[6][minuteCount - 180] = pressure;
  }
  //From 210 to 215 min.
  else if ((minuteCount >= 210) && (minuteCount <= 215)) {
    pressureSamples[7][minuteCount - 210] = pressure;
  }
  //From 240 to 245 min.
  else if ((minuteCount >= 240) && (minuteCount <= 245)) {
    pressureSamples[8][minuteCount - 240] = pressure;
  }


  if (minuteCount == 5) {
    Serial.println("forecast: menos de 5min");
    // Avg pressure in first 5 min, value averaged from 0 to 5 min.
    pressureAvg[0] = ((pressureSamples[0][0] + pressureSamples[0][1]
                       + pressureSamples[0][2] + pressureSamples[0][3]
                       + pressureSamples[0][4] + pressureSamples[0][5]) / 6);
  }
  else if (minuteCount == 35) {
    Serial.println("forecast: primeiros 30min");
    // Avg pressure in 30 min, value averaged from 0 to 5 min.
    pressureAvg[1] = ((pressureSamples[1][0] + pressureSamples[1][1]
                       + pressureSamples[1][2] + pressureSamples[1][3]
                       + pressureSamples[1][4] + pressureSamples[1][5]) / 6);
    float change = (pressureAvg[1] - pressureAvg[0]);
    dP_dt = change / 5;
  }
  else if (minuteCount == 65) {
    Serial.println("forecast: primeira hora");
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[2] = ((pressureSamples[2][0] + pressureSamples[2][1]
                       + pressureSamples[2][2] + pressureSamples[2][3]
                       + pressureSamples[2][4] + pressureSamples[2][5]) / 6);
    float change = (pressureAvg[2] - pressureAvg[0]);
    dP_dt = change / 10;
  }
  else if (minuteCount == 95) {
    Serial.println("forecast: uma hora e meia");
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[3] = ((pressureSamples[3][0] + pressureSamples[3][1]
                       + pressureSamples[3][2] + pressureSamples[3][3]
                       + pressureSamples[3][4] + pressureSamples[3][5]) / 6);
    float change = (pressureAvg[3] - pressureAvg[0]);
    dP_dt = change / 15;
  }
  else if (minuteCount == 125) {
    Serial.println("forecast: duas horas");
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[4] = ((pressureSamples[4][0] + pressureSamples[4][1]
                       + pressureSamples[4][2] + pressureSamples[4][3]
                       + pressureSamples[4][4] + pressureSamples[4][5]) / 6);
    float change = (pressureAvg[4] - pressureAvg[0]);
    dP_dt = change / 20;
  }
  else if (minuteCount == 155) {
    Serial.println("forecast: duas horas e meia");
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[5] = ((pressureSamples[5][0] + pressureSamples[5][1]
                       + pressureSamples[5][2] + pressureSamples[5][3]
                       + pressureSamples[5][4] + pressureSamples[5][5]) / 6);
    float change = (pressureAvg[5] - pressureAvg[0]);
    dP_dt = change / 25;
  }
  else if (minuteCount == 185) {
    Serial.println("forecast: 3 horas");
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[6] = ((pressureSamples[6][0] + pressureSamples[6][1]
                       + pressureSamples[6][2] + pressureSamples[6][3]
                       + pressureSamples[6][4] + pressureSamples[6][5]) / 6);
    float change = (pressureAvg[6] - pressureAvg[0]);
    dP_dt = change / 30;
  }
  else if (minuteCount == 215) {
    Serial.println("forecast: 3 horas e meia");
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[7] = ((pressureSamples[7][0] + pressureSamples[7][1]
                       + pressureSamples[7][2] + pressureSamples[7][3]
                       + pressureSamples[7][4] + pressureSamples[7][5]) / 6);
    float change = (pressureAvg[7] - pressureAvg[0]);
    dP_dt = change / 35;
  }
  else if (minuteCount == 245) {
    Serial.println("forecast: 4 horas");
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[8] = ((pressureSamples[8][0] + pressureSamples[8][1]
                       + pressureSamples[8][2] + pressureSamples[8][3]
                       + pressureSamples[8][4] + pressureSamples[8][5]) / 6);
    float change = (pressureAvg[8] - pressureAvg[0]);
    dP_dt = change / 40; // note this is for t = 4 hour

    minuteCount -= 30;
    pressureAvg[0] = pressureAvg[1];
    pressureAvg[1] = pressureAvg[2];
    pressureAvg[2] = pressureAvg[3];
    pressureAvg[3] = pressureAvg[4];
    pressureAvg[4] = pressureAvg[5];
    pressureAvg[5] = pressureAvg[6];
    pressureAvg[6] = pressureAvg[7];
    pressureAvg[7] = pressureAvg[8];
  }

  minuteCount++;

  if (minuteCount < 36) //if time is less than 35 min
    return 5; // Unknown, more time needed
  else if (dP_dt < (-0.25))
    return 4; // Quickly falling LP, Thunderstorm, not stable
  else if (dP_dt > 0.25)
    return 3; // Quickly rising HP, not stable weather
  else if ((dP_dt > (-0.25)) && (dP_dt < (-0.05)))
    return 2; // Slowly falling Low Pressure System, stable rainy weather
  else if ((dP_dt > 0.05) && (dP_dt < 0.25))
    return 1; // Slowly rising HP stable good weather
  else if ((dP_dt > (-0.05)) && (dP_dt < 0.05))
    return 0; // Stable weather
  else
    return 5; // Unknown
}

/* função para definir se é hora de pegar uma amostra de dados */
boolean IsTimeout()
{
  unsigned long now = millis();
  if (startTime <= now)
  {
    if ( (unsigned long)(now - startTime )  < MILS_IN_MIN )
      return false;
  }
  else
  {
    if ( (unsigned long)(startTime - now) < MILS_IN_MIN )
      return false;
  }

  return true;
}

