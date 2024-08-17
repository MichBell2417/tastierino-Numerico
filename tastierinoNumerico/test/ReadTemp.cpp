#include <Arduino.h>

void setup() {
    Serial.begin(115200);
}

void loop() {
    float temp_celsius = temperatureRead();
    Serial.print("Temperatura interna: ");
    Serial.print(temp_celsius);
    Serial.println(" °C");
    delay(1000);
}