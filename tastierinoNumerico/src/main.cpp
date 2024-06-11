#include <Arduino.h>
#include <Keypad.h>
#include <AccelStepper.h>
#include <WiFi.h>
#include "time.h"
#include "esp_sntp.h"

byte pinCicalino = 13;

byte sensoreMagnetico = 35;

byte ledRosso = 21;
byte ledGiallo = 4;
byte ledVerde = 23;
byte numeroLED = 3;
byte pinLED[] = {ledRosso, ledGiallo, ledVerde};

byte pinUnoStepper = 17;
byte pinDueStepper = 16;
byte pinTreStepper = 15;
byte pinQuattroStepper = 2;

AccelStepper stepper(AccelStepper::FULL4WIRE, pinUnoStepper, pinTreStepper, pinDueStepper, pinQuattroStepper, true);

String codiceIngress = "12345";

boolean statoPortaChiusura = false; // porta aperta

const String ssid = "Vodafone-ESP32&Co";
const String password = "ForYouAndESP32";

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3"; // regole per l'orario
struct tm orario;

const byte ROWS = 4; // four rows
const byte COLS = 3; // four columns
// define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {12, 14, 27, 26};
byte colPins[COLS] = {25, 33, 32};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//variabili temporali
long tempoMaggioreOrario;
long tempoPassatoOrario=0;

// metodi
void bip(int durata);
void setPorta(boolean stato);
boolean getPorta();

void setup(){
  pinMode(pinCicalino, OUTPUT);
  pinMode(sensoreMagnetico, INPUT);
  Serial.begin(115200);
  // Check dei LED
  for (int i = 0; i < numeroLED; i++){
    pinMode(pinLED[i], OUTPUT);
    digitalWrite(pinLED[i], HIGH);
    delay(100);
  }
  delay(100);
  for (int i = numeroLED; i >= 0; i--){
    digitalWrite(pinLED[i], LOW);
    delay(100);
  }
  if (statoPortaChiusura){
    digitalWrite(ledRosso, HIGH);
  }else{
    digitalWrite(ledVerde, HIGH);
  }
  // motore
  stepper.setCurrentPosition(0);
  stepper.setAcceleration(650);
  stepper.setMaxSpeed(1000);
  // Orario
  configTzTime(time_zone, ntpServer1, ntpServer2);
  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnesso!");
}

void loop(){
  tempoMaggioreOrario=millis();
  //aggiorniamo l'orario ogni 1000 millisecondi
  if(tempoMaggioreOrario-tempoPassatoOrario>=1000){
    if (!getLocalTime(&orario)){
      Serial.println("tempo non disponibile");
    }
    tempoPassatoOrario=millis();
  }
  // porta chiusa
  if (getPorta()){
    char customKey = customKeypad.getKey();
    // chiudiamo la porta a chiave
    if (customKey == '#' && !statoPortaChiusura){
      setPorta(true);
    }
    // si vuole inserire il codice
    if (customKey == '*' && statoPortaChiusura){
      boolean inserimento = true;
      digitalWrite(ledRosso, LOW);
      digitalWrite(ledVerde, LOW);
      boolean statoLedGiallo = LOW;
      String code = "";
      Serial.println("inserisci il codice.");
      long tempoPassatoLampeggio = 0;
      while (code.length() < 5 && inserimento){
        // lampeggiamento led giallo
        long tempoMaggioreLampeggio = millis();
        if (tempoMaggioreLampeggio - tempoPassatoLampeggio > 200){
          statoLedGiallo = !statoLedGiallo;
          digitalWrite(ledGiallo, statoLedGiallo);
          tempoPassatoLampeggio = millis();
        }
        // costruzzione codice da verificare
        customKey = customKeypad.getKey();
        if (customKey && customKey != '*' && customKey != '#'){
          code += customKey;
          bip(150);
          Serial.print("*");
        }else if (customKey == '*'){
          // interrompiamo la costruzione del codice
          inserimento = false;
        }
      }
      Serial.print("\n il  . ");
      Serial.print(code);
      digitalWrite(ledGiallo, LOW);
      if (code.length() == 5){
        if (code == codiceIngress){
          setPorta(false);
          Serial.println(" corrisponde");
        }else{
          digitalWrite(ledRosso, HIGH);
          Serial.println(" non corrisponde");
          delay(150);
          bip(200);
        }
      }else{
        digitalWrite(ledRosso, HIGH);
        Serial.println(" il codice è troppo corto");
        bip(200);
        delay(150);
        bip(200);
      }
    }
  }
}

// metodo che emette un suono per la durata inserita
//@param durata tempo in millisecondi
void bip(int durata){
  byte ora= orario.tm_hour;
  if(ora>13 && ora<18){
    digitalWrite(pinCicalino, HIGH);
    delay(durata);
    digitalWrite(pinCicalino, LOW);
    delay(10);
  }
  Serial.println(ora);
}

// viene utilizzato per aprire o chiudere la porta a chiave controllando il motorino
//@param stato con true la porta si chiude, con false la porta si apre
void setPorta(boolean stato){
  int posizione;
  if (stato){
    Serial.println("chiusura porta");
    statoPortaChiusura = true;
    posizione = -2800;
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledRosso, HIGH);
    bip(150);
  }else{
    Serial.println("apertura porta");
    statoPortaChiusura = false;
    posizione = 2800;
    digitalWrite(ledRosso, LOW);
    digitalWrite(ledVerde, HIGH);
    bip(150);
  }
  stepper.move(posizione);
  // spostiamo lo stepper fino a quando non ha raggiunto la posizione
  int posizioneIniziale = stepper.currentPosition();
  while (stepper.currentPosition() != posizione + posizioneIniziale){
    stepper.run();
  }
  digitalWrite(pinUnoStepper, LOW);
  digitalWrite(pinDueStepper, LOW);
  digitalWrite(pinTreStepper, LOW);
  digitalWrite(pinQuattroStepper, LOW);
}
// restituisce lo stato della porta se è aperta o chiusa ma non a chiave.
//@return true=chiusa; false=aperta
boolean getPorta(){
  return !digitalRead(sensoreMagnetico);
}