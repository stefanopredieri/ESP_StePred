#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>

#include "oled_utils.h"

#define SDA 21
#define SCL 22
#define OLED_RST -1
#define ROW 64
#define COL 128
#define BMP280_ADDR 0x77

Adafruit_SSD1306 display = Adafruit_SSD1306(COL, ROW, &Wire, OLED_RST);
Adafruit_BMP280 bmp;


typedef int32_t  BMP280_S32_t;
typedef uint32_t BMP280_U32_t;
typedef int64_t  BMP280_S64_t;

uint16_t dig_T1;
int16_t  dig_T2;
int16_t  dig_T3;

uint16_t dig_P1;
int16_t  dig_P2;
int16_t  dig_P3;
int16_t  dig_P4;
int16_t  dig_P5;
int16_t  dig_P6;
int16_t  dig_P7;
int16_t  dig_P8;
int16_t  dig_P9;

BMP280_S32_t t_fine;

BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T)
{
  BMP280_S32_t var1, var2, T;
  var1 = ((((adc_T >> 3) - ((BMP280_S32_t)dig_T1 << 1))) * ((BMP280_S32_t)dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((BMP280_S32_t)dig_T1)) * ((adc_T >> 4) - ((BMP280_S32_t)dig_T1))) >> 12) * ((BMP280_S32_t)dig_T3)) >> 14;
  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T;
}

BMP280_U32_t bmp280_compensate_P_int64(BMP280_S32_t adc_P)
{
  BMP280_S64_t var1, var2, p;
  var1 = ((BMP280_S64_t)t_fine) - 128000;
  var2 = var1 * var1 * (BMP280_S64_t)dig_P6;
  var2 = var2 + ((var1 * (BMP280_S64_t)dig_P5) << 17);
  var2 = var2 + (((BMP280_S64_t)dig_P4) << 35);
  var1 = ((var1 * var1 * (BMP280_S64_t)dig_P3) >> 8) + ((var1 * (BMP280_S64_t)dig_P2) << 12);
  var1 = (((((BMP280_S64_t)1) << 47) + var1)) * ((BMP280_S64_t)dig_P1) >> 33;
  if (var1 == 0)
  {
    return 0; // avoid exception caused by division by zero
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((BMP280_S64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((BMP280_S64_t)dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((BMP280_S64_t)dig_P7) << 4);
  return (BMP280_U32_t)p;
}


void leggiParametriCalibrazione() {
  uint8_t calibratori[24];

  Wire.beginTransmission(BMP280_ADDR);
  Wire.write(0x88);
  Wire.endTransmission();

  Wire.requestFrom(BMP280_ADDR, 24);
  if (Wire.available() == 24) {
    for (int i = 0; i < 24; i++) {
      calibratori[i] = Wire.read();
    }
  }
  dig_T1 = (uint16_t)((calibratori[1] << 8) | calibratori[0]);
  dig_T2 = (int16_t)((calibratori[3] << 8) | calibratori[2]);
  dig_T3 = (int16_t)((calibratori[5] << 8) | calibratori[4]);
  dig_P1 = (uint16_t)((calibratori[7] << 8) | calibratori[6]);
  dig_P2 = (int16_t)((calibratori[9] << 8) | calibratori[8]);
  dig_P3 = (int16_t)((calibratori[11] << 8) | calibratori[10]);
  dig_P4 = (int16_t)((calibratori[13] << 8) | calibratori[12]);
  dig_P5 = (int16_t)((calibratori[15] << 8) | calibratori[14]);
  dig_P6 = (int16_t)((calibratori[17] << 8) | calibratori[16]);
  dig_P7 = (int16_t)((calibratori[19] << 8) | calibratori[18]);
  dig_P8 = (int16_t)((calibratori[21] << 8) | calibratori[20]);
  dig_P9 = (int16_t)((calibratori[23] << 8) | calibratori[22]);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);

  bmp.begin(BMP280_ADDR);
  
  leggiParametriCalibrazione();

  if (!oled_init(display, 0x3C, 2, 2)) {   
    Serial.println("OLED non trovato");
    while (true) delay(10);
  }

  oled_riga("Avvio...", 1, CENTRO);
  oled_aggiorna();
}

void loop() {
  byte data[6];

  Wire.beginTransmission(BMP280_ADDR);
  Wire.write(0xF7);
  Wire.endTransmission();

  Wire.requestFrom(BMP280_ADDR, 6);
  if (Wire.available() == 6) {
    for (int i = 0; i < 6; i++) {
      data[i] = Wire.read();
    }
  }
  
  int32_t pressure_grezza = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)(data[2] >> 4));
  int32_t temperature_grezza = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)(data[5] >> 4));


  BMP280_S32_t T_comp = bmp280_compensate_T_int32(temperature_grezza);
  BMP280_U32_t P_comp = bmp280_compensate_P_int64(pressure_grezza);


  float T = T_comp / 100.0;
  float P = P_comp / 25600.0;

  oled_pulisci();
  oled_titolo("BMP280", 2);
  oled_valore("T=", String(T, 2) + " C", 1);
  oled_valore("P=", String(P, 2) + " hPa", 1);
  oled_aggiorna();

  Serial.print("Temperatura=");
  Serial.println(T, 2);
  Serial.print("Pressione=");
  Serial.println(P, 2);

  delay(1000);
}