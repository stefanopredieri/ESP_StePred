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

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA, SCL);

  bmp.begin(BMP280_ADDR);

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

    oled_pulisci();
    oled_titolo("BMP280", 2);
    oled_riga("Temperatura grezza: ", 1);
    oled_riga(String(temperature_grezza, 2) , 1);
    oled_riga("Pressione grezza: ", 1);
    oled_riga(String(pressure_grezza, 2), 1);
    oled_aggiorna();
    Serial.print("Temperatura grezza: ");
    Serial.println(temperature_grezza, 2);
    Serial.print("Pressione grezza: ");
    Serial.println(pressure_grezza, 2);
   

    delay(1000);
}
