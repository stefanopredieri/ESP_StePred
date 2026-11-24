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

    float t = (temperature_grezza * 0.00012f) - 40.0f;

    float p = (pressure_grezza * 0.00000762939453125) + 300.0f;

    oled_pulisci();
    oled_titolo("BMP280", 1);
    oled_valore("Temperatura", String(t, 2) + " C", 1);
    oled_valore("Pressione", String(p, 2) + " hPa", 1);
    oled_aggiorna();
    Serial.print("Temperatura: ");
    Serial.print(t, 2);
    Serial.print(" C, Pressione: ");
    Serial.print(p, 2);
    Serial.println(" hPa");

    delay(1000);
}
