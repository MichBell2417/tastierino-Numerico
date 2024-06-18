#include <Arduino.h>
#include <Keypad.h>
#include <AccelStepper.h>
#include <WiFi.h>
#include "time.h"
#include "esp_sntp.h"
#include <EEPROM.h>

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

boolean statoPortaChiusura;

const String ssid = "Vodafone-ESP32&Co";
const String password = "ForYouAndESP32";

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
// regole per l'orario
const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3"; 
struct tm orario;

//celle contenenti la password amministratore
int pAdmin[]={1,2,3,4,5};

//celle contenenti la password ospite
int pGuest[]={7,8,9,10,11};

//celle contenenti lo stato della poorta
int statoPorta=13;

const byte ROWS = 4; // four rows
const byte COLS = 3; // four columns
// define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {33, 12, 14, 26};
byte colPins[COLS] = {25, 32, 27};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//variabili temporali
long tempoMaggioreOrario;
long tempoPassatoOrario=0;

// metodi
void bip(int durata);
void setPorta(bool stato);
bool getPorta();
String digitazioneCodice(int numeroCaratteri);
int checkPassword(String passwordToCheck);
void changePassword(String nuovaPassword, int passwordToChange);

void setup(){
  EEPROM.begin(15);
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
  Serial.println(EEPROM.read(statoPorta));
  statoPortaChiusura=EEPROM.read(statoPorta);
  if(statoPortaChiusura){
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
    digitalWrite(ledGiallo, HIGH);
  }
  digitalWrite(ledGiallo, LOW);
  Serial.println("\nConnesso!");
}
void loop(){
  tempoMaggioreOrario=millis();
  //aggiorniamo l'orario ogni 1000 millisecondi
  if(tempoMaggioreOrario-tempoPassatoOrario>=1000){
    if (!getLocalTime(&orario)){
      Serial.println("tempo non disponibile");
    }
    Serial.println(&orario, "%H:%M:%S");
    tempoPassatoOrario=millis();
  }
  char customKey = customKeypad.getKey();
  // porta chiusa
  if (getPorta()){
    // chiudiamo la porta a chiave
    if (customKey == '#' && !statoPortaChiusura){
      setPorta(true);
    }
    // si vuole inserire il codice
    if (customKey == '*' && statoPortaChiusura){
      digitalWrite(ledRosso, LOW);
      digitalWrite(ledVerde, LOW);
      String code = digitazioneCodice(5);
      Serial.print("\n il codice è: ");
      Serial.print(code);
      if (code.length() == 5){
        if(checkPassword(code)!=-1){
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
  }else{
    //se la porta è aperta controlliamo se si vuole cambiare password
    if(customKey=='*'){
      String code=digitazioneCodice(5);
      //se abbiamo inserito la password amministratore
      if(checkPassword(code)==1){
        bip(300);
        //chiede a che utente cambiare password
        int utente=atoi(digitazioneCodice(1).c_str()); //dovra essere 1 carattere
        bip(150);
        delay(100);
        bip(150);
        //nuovoCodice
        code=digitazioneCodice(5);
        changePassword(code,utente);
      }
    }
  }
}
//si legge la digitazione del codice a cinque caratteri
//@param numeroCaratteri la lunghezza del codice aspettata
//@return restituisce il codice della lunghezza desiderata digitato
String digitazioneCodice(int numeroCaratteri){
  String code = "";
  bool inserimento = true;
  Serial.println("inserisci il codice:");
  long tempoPassatoLampeggio = 0;
  bool statoLedGiallo = LOW;
  while (code.length() < numeroCaratteri && inserimento){
    // lampeggiamento led giallo
    long tempoMaggioreLampeggio = millis();
    if (tempoMaggioreLampeggio - tempoPassatoLampeggio > 200){
      statoLedGiallo = !statoLedGiallo;
      digitalWrite(ledGiallo, statoLedGiallo);
      tempoPassatoLampeggio = millis();
    }
    // costruzzione codice da verificare
    char charInserito = customKeypad.getKey();
    if (charInserito && charInserito != '*' && charInserito != '#'){
      code += charInserito;
      bip(150);
      Serial.print("*");
    }else if (charInserito == '*'){
      // interrompiamo la costruzione del codice
      inserimento = false;
    }
  }
  digitalWrite(ledGiallo, LOW);
  return code;
}

//metodo che cambia le password nella EEPROM
//@param nuovaPassword è la password da scivere nella EEPROM
//@param passwordToChange è la password da sostituire nella EEPROM 1=Amministratore 2=Ospite
void changePassword(String nuovaPassword, int passwordToChange){
  if(nuovaPassword.length()==5){
    if(passwordToChange==1){
      for(int i=0; i<5; i++){
        char numero=nuovaPassword[i];
        EEPROM.write(pGuest[i], uint8_t(numero));
        EEPROM.commit();
      }
    }else if(passwordToChange==2){
      for(int i=0; i<5; i++){
        char numero=nuovaPassword[i];
        EEPROM.write(pGuest[i], uint8_t(numero));
        EEPROM.commit();
      }
    }
  }
  Serial.println("password cambiata");
  bip(100);
  delay(100);
  bip(100);
  delay(100);
  bip(100);
}

//metodo che controlla la corrispondena delle password nella EEPROM 
//@param passwordToCheck è la password da verificare 
//@return -1=nessuna corrispondenza, 1=password amministratore, 2=password Guest
int checkPassword(String passwordToCheck){
  bool corrispondeAdmin=true, corrispondeGuest=true;
  for(int i=0; i<5; i++){
    char numero=passwordToCheck[i];
    Serial.print("numero:");
    Serial.println(numero);
    Serial.print("admin:");
    Serial.println(EEPROM.read(pAdmin[i])-48);
    Serial.print("guest:");
    Serial.println(EEPROM.read(pGuest[i])-48);
    if(numero!=EEPROM.read(pAdmin[i]) && corrispondeAdmin){
      corrispondeAdmin=false;
      Serial.println("cabiato admin");
    }
    if(numero!=EEPROM.read(pGuest[i]) && corrispondeGuest){
      corrispondeGuest=false;
      Serial.println("cabiato guest");
    }
  }
  if(corrispondeAdmin){
    return 1;
  }else if(corrispondeGuest){
    return 2;
  }else{
    return -1;
  }
}

// metodo che emette un suono per la durata inserita
//@param durata tempo in millisecondi
void bip(int durata){
  byte ora= orario.tm_hour;
  //[12;18[ incluse le ore 11 ma non le ore 18
  if(ora>=12 && ora<19){
    digitalWrite(pinCicalino, HIGH);
    delay(durata);
    digitalWrite(pinCicalino, LOW);
    delay(10);
  }
  Serial.println(ora);
}

// viene utilizzato per aprire o chiudere la porta a chiave controllando il motorino
//@param stato con true la porta si chiude, con false la porta si apre
void setPorta(bool stato){
  int posizione;
  if (stato){
    Serial.println("chiusura porta");
    statoPortaChiusura = true;
    posizione = -2800;
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledRosso, HIGH);
    //salviamo nella EEPROM lo sato della porta
    //EEPROM.write(statoPorta, 1);
    //EEPROM.commit();
    bip(150);
  }else{
    Serial.println("apertura porta");
    statoPortaChiusura = false;
    posizione = 2800;
    digitalWrite(ledRosso, LOW);
    digitalWrite(ledVerde, HIGH);
    //salviamo nella EEPROM lo sato della porta
    //EEPROM.write(statoPorta, 0);
    //EEPROM.commit();
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
bool getPorta(){
  return !digitalRead(sensoreMagnetico);
}