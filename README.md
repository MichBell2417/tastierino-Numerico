# tastierino-Numerico
## Descrizione
questa repository contiene il codice con i file compilati per ESP-32, utilizzati per realizzare un tastierino numerico, utile per controllare gli accessi ad una stanza o edificio. Oltre al codice, qui, sarà anche possibile trovare i schemi elettrici. 
## Versioni
esistono tre versioni di schemi elettrici:
### V. con motore DC 1.0
questa è la versione antenata che al posto del motore stepper ha un motore DC che pero a causa della sua velocita ed il poco controllo su questa rende difficile il controllo del chiavistello.
### V. senza sensore RFID 2.0
Questa versione non che quella provata e testata con il codice fornito è quella che ho realizzato nella pratica.
### V. con RFID 2.1
attualmente non testata e non applicata non è stato inserito alcun codice a riguardo, solo lo schema elettrico.
## Organizzazione Repository
la repositori contiene attualmente due cartelle "tastierinoNumerico" e "Schema-tastierino-numerico", nella seconda possiamo trovare gli schemi elettrici seguiti per la realizzazione dei circuiti e la prima invece è un progetto realizzato con **PlatformIO** dove possiamo trovare librerie in "lib" e i codici in "src".
## last update:
Il progetto dal commit [ccebbc7](https://github.com/MichBell2417/tastierino-Numerico/commit/ccebbc7bf8c14d1bc89ffaccd8e1e525e4cb6fa8) è controllato anche dalla pagina web del mio sito personale riguardante IoT. Tutto questo grazie all'integrazione di mqtt con python e c++ ma anche python e websocket presenti nella repository [ServerWebSocketSSL](https://github.com/MichBell2417/ServerWebSocketSSL).
