#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

int pinSensorePIR = 34;

void setup(){
    Serial.begin(115200);
    pinMode(pinSensorePIR, INPUT);
    lcd.init(); // initialize the lcd
    // Print a message to the LCD.
    lcd.setCursor(0, 0);
    lcd.print("STATO:");
    lcd.setCursor(0, 1);
    lcd.print("connessione...");
    delay(1000);
    lcd.setCursor(0,0);
    lcd.print(13);
    lcd.setCursor(2,0);
    lcd.print(":");
    lcd.setCursor(3,0);
    lcd.print(49);
    lcd.setCursor(5,0);
    lcd.print(":");
    lcd.setCursor(6,0);
    lcd.print(13);
}

void loop(){
    if (digitalRead(pinSensorePIR)){
        long tempoStart = millis();
        long tempoSecondo = millis();
            Serial.println("rilevata presenza");
        while (digitalRead(pinSensorePIR)){
            lcd.backlight();
            tempoSecondo = millis();
        }
        Serial.print("il tempo di attività è: ");
        Serial.println((tempoSecondo-tempoStart)/1000.0);
    }

    lcd.noBacklight();
}
