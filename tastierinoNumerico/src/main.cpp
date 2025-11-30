#include <Arduino.h>
#include <Keypad.h>
#include <AccelStepper.h>
#include <WiFi.h>
#include <PubSubClient.h> //per poter relazionare con il brocker mqtt
#include "time.h"
#include "esp_sntp.h"
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "LED.cpp"

LiquidCrystal_I2C screen(0x27, 16, 2);

byte pinSensorePIR = 34;

byte pinCicalino = 13;

byte sensoreMagnetico = 35;

byte pinInterruttoreInterno = 18;

LED ledRosso(21);
LED ledGiallo(23);
LED ledVerde(22);
byte numeroLED = 3;
LED pinLED[] = {ledRosso, ledGiallo, ledVerde};

byte pinUnoStepper = 17;
byte pinDueStepper = 16;
byte pinTreStepper = 15;
byte pinQuattroStepper = 2;

AccelStepper stepper(AccelStepper::FULL4WIRE, pinUnoStepper, pinTreStepper, pinDueStepper, pinQuattroStepper, true);

/**indica se la porta è chiusa a chiave
 * @return porta chiusa=true, porta aperta=false
 */
boolean statoPortaChiusura;


const char* ssid = "Vodafone-C01830055";
const char* password = "Ytnackqmz9P3cags";
const char* server_ip = "192.168.1.19";

//configuro il client mqtt
WiFiClient espClient;
PubSubClient client(espClient);

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

//numero che indica il numero di volte che il codice è errato
int reiterazioneErrore=0;

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
long tempoMaggioreLampeggio;
long tempoPassatoLampeggio=0;
//variabile che indica il tempo che è passato dall'ingresso nel ciclo della digitazione del pin
long tempoPassatoIngressoDigitazione=0;

//indica l'ora
byte ora;

// metodi
void bip(int durata);
void setPorta(bool stato);
bool getPorta();
String digitazioneCodice(int numeroCaratteri, int nLED);
String digitazioneCodice(int numeroCaratteri);
int checkPassword(String passwordToCheck);
void changePassword(String nuovaPassword, int passwordToChange);
void interrompiLoop();
void lampaeggiaLed(int nLed);
//metodi per gestire comunicazione con server
void callback(char* topic, byte* message, unsigned int length);
void checkWiFi();
void reconnect();
void executeCommand(String command, String who);
String* split(const String &string);
void sendResponse(String message);

void setup(){
  EEPROM.begin(15);
  pinMode(pinCicalino, OUTPUT);
  pinMode(sensoreMagnetico, INPUT);
  pinMode(pinInterruttoreInterno, INPUT);
  Serial.begin(115200);
  // LCD Screen 
  screen.init();
  screen.backlight();
  screen.setCursor(0, 0);
  screen.print("STATO:");
  // Check dei LED
  for (int i = 0; i < numeroLED; i++){
    pinMode(pinLED[i].getPin(), OUTPUT);
    digitalWrite(pinLED[i].getPin(), HIGH);
    delay(100);
  }
  delay(100);
  for (int i = numeroLED; i >= 0; i--){
    digitalWrite(pinLED[i].getPin(), LOW);
    delay(100);
  }
  Serial.println(EEPROM.read(statoPorta));
  statoPortaChiusura=EEPROM.read(statoPorta);
  if(statoPortaChiusura){
    digitalWrite(ledRosso.getPin(), HIGH);
  }else{
    digitalWrite(ledVerde.getPin(), HIGH);
  }
  // motore
  stepper.setCurrentPosition(0);
  stepper.setAcceleration(650);
  stepper.setMaxSpeed(1000);
  // Orario
  configTzTime(time_zone, ntpServer1, ntpServer2);
  // WiFi
  WiFi.begin(ssid, password);
  screen.setCursor(0, 1);
  screen.print("connessione...");
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
    digitalWrite(ledGiallo.getPin(), HIGH);
  }
  digitalWrite(ledGiallo.getPin(), LOW);
  Serial.println("\nConnesso!");
  screen.clear();
  //mqtt
  client.setServer(server_ip, 1883);
  client.setCallback(callback);
  client.connect("ESP32_PortaMichele");
  client.subscribe("esp32/command");
}
void loop(){
  tempoMaggioreOrario=millis();
  //aggiorniamo l'orario ogni 1000 millisecondi
  if(tempoMaggioreOrario-tempoPassatoOrario>=1000){
    if (!getLocalTime(&orario)){
      Serial.println("tempo non disponibile");
    }
    screen.setCursor(0,0);
    if(orario.tm_hour<10){
      screen.print(0);
      screen.setCursor(1,0);
      screen.print(orario.tm_hour);
    }else{
      screen.print(orario.tm_hour);
    }
    screen.setCursor(2,0);
    screen.print(":");
    screen.setCursor(3,0);
    if(orario.tm_min<10){
      screen.print(0);
      screen.setCursor(4,0);
      screen.print(orario.tm_min);
    }else{
      screen.print(orario.tm_min);
    }
    screen.setCursor(5,0);
    screen.print(":");
    screen.setCursor(6,0);
    if(orario.tm_sec<10){
      screen.print(0);
      screen.setCursor(7,0);
      screen.print(orario.tm_sec);
    }else{
      screen.print(orario.tm_sec);
    }
    screen.setCursor(8,0);
    screen.print("        ");
    //Serial.println(&orario, "%H:%M:%S");
    ora=orario.tm_hour;
    if(ora>=22 || ora<7){
      screen.noBacklight();
    }else{
      screen.backlight();
    }
    tempoPassatoOrario=millis();
    float temperaturaInterna = temperatureRead();
    //Serial.print("temperatura = ");
    //Serial.println(temperaturaInterna);
  }
  char customKey = customKeypad.getKey();
  // porta chiusa
  if (getPorta()){
    //clicchiamo l'interruttore interno
    if(digitalRead(pinInterruttoreInterno)){
      //invertiamo lo stato della chiusura a chiave della porta
      setPorta(!statoPortaChiusura);
      String txt="persona All'interno|switchInterno|";
      statoPortaChiusura ? txt+="ha chiuso la porta" : txt+="ha aperto la porta"; 
      sendResponse(txt);
      delay(200);
    }
    if(statoPortaChiusura){
      screen.setCursor(0,1);
      screen.print("allarme attivo  ");
      // si vuole inserire il codice
      if (customKey == '*'){
        digitalWrite(ledRosso.getPin(), LOW);
        digitalWrite(ledVerde.getPin(), LOW);
        String code = digitazioneCodice(5);
        screen.clear();
        Serial.print("\n il codice è: ");
        Serial.print(code);
        if (code.length() == 5){
          int who = checkPassword(code);
          String txt="";
          if(who!=-1){
            who == 1 ? txt+="Admin" : txt+="Guest";
            txt+="|tastierinoEsterno|apertoPorta con errori pari a "+reiterazioneErrore;
            setPorta(false);
            reiterazioneErrore=0;
            Serial.println(" corrisponde");
            sendResponse(txt);
          }else{
            digitalWrite(ledRosso.getPin(), HIGH);
            reiterazioneErrore++;
            if(reiterazioneErrore==3){
              sendResponse("-1|tastierinoEsterno|attivazioneAllarme errori pari a 3");
              //interrompiamo il loop e scatta l'allarme
              interrompiLoop();
              screen.clear();
              //riportiamo tutti i settaggi alla normalità
              ledRosso.setStato(1);
              digitalWrite(ledRosso.getPin(), ledRosso.getStato());
              digitalWrite(pinCicalino, LOW);
            }
            Serial.println(" non corrisponde");
            delay(150);
            bip(200);
          }
        }else{
          digitalWrite(ledRosso.getPin(), HIGH);
          Serial.println(" il codice è troppo corto");
          bip(200);
          delay(150);
          bip(200);
        }
      }
    }else{
      screen.setCursor(0,1);
      screen.print("allarme inattivo");
      // chiudiamo la porta a chiave
      if (customKey == '#'){
        sendResponse("-1|tastierinoEsterno|chiusuraPorta");
        setPorta(true);
      }
    }
    
  }else if(statoPortaChiusura){
    //se la porta è aperta ma è chiusa a chiave
    sendResponse("-1|porta|attivazioneAllarme apertura porta imprevista");
    interrompiLoop();
    digitalWrite(pinCicalino, LOW);
    setPorta(false);
    screen.clear();
  }else {
    screen.setCursor(0, 1);
    screen.print("porta aperta    ");
    //se la porta è aperta controlliamo se si vuole cambiare password
    if(customKey=='*' && !statoPortaChiusura){
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
  //controllo se il wifi è ancora connesso
  checkWiFi();
  //se il servizio mqtt non è più collegato mi ci ricollego
  if (!client.connected()) {
      reconnect();
  }
  //evito che il client interrompa l'ascolto
  client.loop();
}

//metodi per la gestione della comunicazione con server
/**
 * metodo che viene richiamato alla ricezione di un nuovo messaggio nel topic controllato 
 * dal client
 */
void callback(char* topic, byte* message, unsigned int length) {
    String command = "";
    for (int i = 0; i < length; i++) command += (char)message[i];
    Serial.println("Ricevuto comando: " + command);
    String* component=split(command);
    //Verifico che il comando abbia inviato un pin esistente
    int user=checkPassword(component[0]);
    Serial.println(component[1]);
    if(user==1){
      executeCommand(component[1],"Admin");
    }else if(user=2){
      executeCommand(component[1],"Guest");
    }else{
      sendResponse(component[0]+"|web|"+component[2]);
    }
}
void executeCommand(String command, String who){
  String txt=who+"|web|";
  if(command=="CloseDoor"){
    //chiudi la porta
    setPorta(true);
    if(statoPortaChiusura){
      txt+="chiusura effettuata con successo";
    }else{
      txt+="chiusura non effettuata";
    }
  }else if(command.equals("OpenDoor")){
    //apri la porta
    setPorta(false);
    if(statoPortaChiusura){
      txt+="apertura non eseguita";
    }else{
      txt+="apertura eseguita con successo";
    }
  }else if(command.equals("StatusDoor")){
    //comunica lo stato della porta attualmente
    String comunicazione="";
    if(statoPortaChiusura){
      comunicazione+="la porta sta chiusa a chiave";
    }else{
      if(getPorta()){
        comunicazione+="la porta sta chiusa ma non a chiave";
      }else{
        comunicazione+="la porta sta aperta";
      }
    }
    txt+=comunicazione;
  }
  sendResponse(txt);
  /*else if(command.equals("ChronDoor")){
    //comunica la cronologia degli stati della porta
    //PROBABILE VENGA FATTO DAL SERVER
  }*/
}
void checkWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi perso, riconnessione in corso...");
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.print(".");
        }
        Serial.println("WiFi riconnesso!");
    }
}
/**
 * eseguo la riconnessione al broker mqtt
 */
void reconnect() {
    while (!client.connected()) {
        Serial.print("Tentativo di riconnessione MQTT...");
        if (client.connect("ESP32_Client")) {
            Serial.println("Connesso!");
            client.subscribe("esp32/command");  // Riesegue la sottoscrizione al topic
        } else {
            Serial.print("Tentativo fallito. Codice: ");
            Serial.print(client.state());
            Serial.println(" Riprovo tra 5 secondi...");
            delay(5000);
        }
    }
}

/**
 * interrompe il loop fino a quando non viene scritto il codice amministratore o guest
 * eseguiamo il lampeggio del led ed il suono del cicalino.
*/
 void interrompiLoop(){
  String code="";
  //aspettiamo la digitazione della password amministratore
  while(checkPassword(code)==-1){
    //in questo modo aspettiamo la password amministratore, e facciamo lampeggiare il LED rosso con il cicalino
    code=digitazioneCodice(5, 0);
  }
  String txt="";
  int who=checkPassword(code);
  who == 1 ? txt+="Admin" : txt+="Guest";
  txt+="|tastierinoEsterno|interruzioneAllarme";
  sendResponse(txt);
}

//metodo che fa lampeggiare un led. Va richiamato sempre in un ciclo.
//@param nLed è il numero che corrispoinde al LED che si vuole far lampeggiare nel vettore pinLED 
void lampaeggiaLed(int nLed){
  pinLED[nLed].setStato(!pinLED[nLed].getStato());
  digitalWrite(pinLED[nLed].getPin(), pinLED[nLed].getStato());
  //accendiamo il cicalino in sincronia con il LED rosso
  if(nLed==0 && !(ora >= 21 || ora <= 9)){
    digitalWrite(pinCicalino, pinLED[nLed].getStato());
  }
}

//si legge la digitazione del codice a cinque caratteri
//@param numeroCaratteri la lunghezza del codice aspettata
//@param nLED indica la posizione del LED nel vettore pinLED
//@return restituisce il codice della lunghezza desiderata digitato
String digitazioneCodice(int numeroCaratteri, int nLED){
  String code = "";
  bool inserimento = true;
  screen.clear();
  screen.setCursor(0,0);
  screen.print("insert password");
  Serial.println("inserisci il codice:");
  tempoPassatoIngressoDigitazione=millis();
  int nColonnaSchermo=0;
  while (code.length() < numeroCaratteri && inserimento ){
    // lampeggiamento led giallo
    tempoMaggioreLampeggio = millis();
    if (tempoMaggioreLampeggio - tempoPassatoLampeggio > 200){
      lampaeggiaLed(nLED); 
      tempoPassatoLampeggio=millis();
    }
    //se il led che lampeggia corrisponde al rosso (0)
    if(nLED==0){
      if(digitalRead(pinInterruttoreInterno)){
        for(int i=0; i<5; i++){
          code+=(char)(EEPROM.read(pGuest[i]));
        }
        return code;
      }
    }
    // costruzione codice da verificare
    char charInserito = customKeypad.getKey();
    if (charInserito && charInserito != '*' && charInserito != '#'){
      code += charInserito;
      if(nLED!=0){
        bip(150);
      }
      screen.setCursor(nColonnaSchermo,1);
      screen.print("*");
      nColonnaSchermo++;
      Serial.print("*");
    }else if (charInserito == '*' || millis()-tempoPassatoIngressoDigitazione>=8000){
      //interrompiamo la costruzione del codice
      inserimento = false;
    }
  }
  digitalWrite(ledGiallo.getPin(), LOW);
  return code;
}

String digitazioneCodice(int numeroCaratteri){
  return digitazioneCodice(numeroCaratteri, 1);
}

//metodo che cambia le password nella EEPROM
//@param nuovaPassword è la password da scivere nella EEPROM
//@param passwordToChange è la password da sostituire nella EEPROM 1=Amministratore 2=Ospite
void changePassword(String nuovaPassword, int passwordToChange){
  if(nuovaPassword.length()==5){
    if(passwordToChange==1){
      for(int i=0; i<5; i++){
        char numero=nuovaPassword[i];
        EEPROM.write(pAdmin[i], uint8_t(numero));
        EEPROM.commit();
      }
      sendResponse("Admin|tastierinoNumerico|cambiamentoPasswordAdmin");
    }else if(passwordToChange==2){
      for(int i=0; i<5; i++){
        char numero=nuovaPassword[i];
        EEPROM.write(pGuest[i], uint8_t(numero));
        EEPROM.commit();
        sendResponse("Admin|tastierinoNumerico|cambiamentoPasswordGuest");
      }
    }else{
      sendResponse("Admin|tastierinoNumerico|password da cambiare inesistente");
    }
  }
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
      Serial.println("errato admin");
    }
    if(numero!=EEPROM.read(pGuest[i]) && corrispondeGuest){
      corrispondeGuest=false;
      Serial.println("errato guest");
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
  //[12;18[ incluse le ore 12 ma non le ore 18
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
  if(getPorta()){ //solo se la porta è chiusa
    if (stato){
        Serial.println("chiusura porta");
        screen.setCursor(0,0);
        screen.print("chiusura...");
        statoPortaChiusura = true;
        posizione = -2400;
        digitalWrite(ledVerde.getPin(), LOW);
        //salviamo nella EEPROM lo sato della porta
        //EEPROM.write(statoPorta, 1);
        //EEPROM.commit();
        //bip(150);
      }else{
        Serial.println("apertura porta");
        screen.setCursor(0,0);
        screen.print("apertura...");
        statoPortaChiusura = false;
        posizione = 2400;
        digitalWrite(ledRosso.getPin(), LOW);
        //salviamo nella EEPROM lo sato della porta
        //EEPROM.write(statoPorta, 0);
        //EEPROM.commit();
        //bip(150);
      }
  }
  
  stepper.move(posizione);
  // spostiamo lo stepper fino a quando non ha raggiunto la posizione
  int posizioneIniziale = stepper.currentPosition();
  while (stepper.currentPosition() != posizione + posizioneIniziale){
    stepper.run();
  }
  //accendiamo i LED per indicare lo stato dell aporta all'esterno
  if(posizione>0){
    digitalWrite(ledVerde.getPin(), HIGH);
  }else{
    digitalWrite(ledRosso.getPin(), HIGH);
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
/**
 * funzione che data una stringa utilizza il carattere "|" per separare la stringa
 * @return un vettore di stringhe
 */
String* split(const String &string){
    int posizione = 0;
    String content = "";
    static String message[3] = {"", "", ""};
    for (int i = 0; i < string.length(); i++){
        if (string[i] == '|'){
            if (posizione < 3) {
                message[posizione] = content;
                posizione++;
            }
            content = "";
        } else {
            content += string[i];
        }
    }
    return message;
}
/**
 * inviamo i feedback al brocker mqtt
 */
void sendResponse(String message){
  client.publish("esp32/response",message.c_str());
}