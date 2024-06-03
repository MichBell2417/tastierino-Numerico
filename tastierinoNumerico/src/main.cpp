#include <Arduino.h>
#include <Keypad.h>

byte pinCicalino=13;

byte sensoreMagnetico=35;

byte ledRosso=21;
byte ledGiallo=22;
byte ledVerde=23;
byte numeroLED=3;
byte pinLED[]={ledRosso, ledGiallo, ledVerde};

byte pinUnoMotorino=16;
byte pinDueMotorino=17;

String codiceIngress="12345";

boolean statoPortaChiusura=false; //porta aperta

const byte ROWS = 4; //four rows
const byte COLS = 3; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {12,14,27,26};
byte colPins[COLS] = {25, 33, 32};

Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

//metodi
void bip(int durata);
void setPorta(boolean stato);
boolean getPorta();

void setup(){
  pinMode(pinCicalino, OUTPUT);
  pinMode(pinUnoMotorino, OUTPUT);
  pinMode(pinDueMotorino, OUTPUT);
  pinMode(sensoreMagnetico, INPUT);
  Serial.begin(115200);
  //svolgiamo il check dei led
  for(int i=0; i<numeroLED; i++){
    pinMode(pinLED[i], OUTPUT);
    digitalWrite(pinLED[i], HIGH);
    delay(100);
  }
  delay(100);
  for(int i=numeroLED; i>=0; i--){
    digitalWrite(pinLED[i], LOW);
    delay(100);
  }
  if(statoPortaChiusura){
    digitalWrite(ledRosso, HIGH);
  }else{
    digitalWrite(ledVerde, HIGH);
  }
}
  
void loop(){
  //porta chiusa
  if(getPorta()){
    char customKey = customKeypad.getKey();
    //chiudiamo la porta a chiave
    if (customKey=='#' && !statoPortaChiusura){
      setPorta(true);
    }
    //si vuole inserire il codice
    if (customKey=='*' && statoPortaChiusura){
      boolean inserimento=true;
      digitalWrite(ledRosso, LOW);
      digitalWrite(ledVerde, LOW);
      boolean statoLedGiallo=LOW;
      String code="";
      Serial.println("inserisci il codice.");
      long tempoPassatoLampeggio=0;
      while(code.length()<5 && inserimento){
        //lampeggiamento led giallo
        long tempoMaggioreLampeggio=millis();
        if(tempoMaggioreLampeggio-tempoPassatoLampeggio>200){
          statoLedGiallo=!statoLedGiallo;
          digitalWrite(ledGiallo, statoLedGiallo);
          tempoPassatoLampeggio=millis();
        }
        //costruzzione codice da verificare
        customKey=customKeypad.getKey();
        if(customKey && customKey!='*' && customKey!='#'){
          code+=customKey;
          bip(150);
          Serial.print("*");
        }else if(customKey=='*'){
          inserimento=false;
        }
      }
      Serial.print("\n il codice inserito è: ");
      Serial.print(code);
      digitalWrite(ledGiallo, LOW);
      if(code.length()==5){
        if(code==codiceIngress){
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

//metodo che emette un suono per la durata inserita
//@param durata tempo in millisecondi
void bip(int durata){
  digitalWrite(pinCicalino, HIGH);
  delay(durata);
  digitalWrite(pinCicalino, LOW);
  delay(10);
}

//viene utilizzato per aprire o chiudere la porta a chiave controllando il motorino
//@param stato con true la porta si chiude, con false la porta si apre
void setPorta(boolean stato){
  if(stato){
    Serial.println("chiusura porta");
    digitalWrite(pinUnoMotorino, HIGH);
    digitalWrite(pinDueMotorino, LOW);
    delay(3000);
    digitalWrite(pinUnoMotorino, LOW);
    digitalWrite(pinDueMotorino, LOW);
    statoPortaChiusura=stato;
    digitalWrite(ledVerde,LOW);
    digitalWrite(ledRosso,HIGH);
    bip(200);
  }else{
    Serial.println("apertura porta");
    digitalWrite(pinUnoMotorino, LOW);
    digitalWrite(pinDueMotorino, HIGH);
    delay(3000);
    digitalWrite(pinUnoMotorino, LOW);
    digitalWrite(pinDueMotorino, LOW);
    statoPortaChiusura=stato;
    digitalWrite(ledRosso,LOW);
    digitalWrite(ledVerde,HIGH);
    bip(200);
  }
}
//restituisce lo stato della porta se è aperta o chiusa ma non a chiave.
//@return true=chiusa; false=aperta
boolean getPorta(){
  return !digitalRead(sensoreMagnetico);
}