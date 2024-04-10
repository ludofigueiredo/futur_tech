
//Integration de librairie externes
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti WiFiMulti;

#include <ArduinoJson.h>

///// 🛜 WIFI (variables) /////
  const char* ssid = "ESP8266-Access-Point";
  const char* password = "123456789";

  String coordinates;

  //Manage asynhronous request
  unsigned long previousMillis = 0;
  const long interval = 100; 

///// 💡LED (variables) //////
  const int pinControlC = D6;     // Pin de commande C du CD4066 --> Axe X
  const int pinControlD = D7;     // Pin de commande D du CD4066 --> Axe Y
  const int joystickButtonPin = D1; // Broche digitale pour lire le bouton du joystick

  //On centre les coordonnées de X et Y pour avoir un range d'environ -512 jusqu'à +512 (au lieu de 0 1024)
  int neutralX = 545;  // Valeur neutre de l'axe X
  int neutralY = 545;  // Valeur neutre de l'axe Y

  int prevJoystickX = 0;  // Valeur précédente de l'axe X
  int prevJoystickY = 0;  // Valeur précédente de l'axe Y

void setup() {
  Serial.begin(74880);
  Serial.println();
  delay(100);

  ///// 🏎️ MOTEURS /////
    pinMode(pinControlC, OUTPUT);
    pinMode(pinControlD, OUTPUT);
    pinMode(joystickButtonPin,INPUT_PULLUP);


  ///// 🛜 WIFI (variables) /////
    // Connexion au réseau Wi-Fi
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(400);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("Connected to WiFi");

}

void loop() {
  ///// 🖥️Joystick /////
  // Activer le CD4066 pour lire l'axe X (connecté à A0)
  digitalWrite(pinControlC, HIGH);
  digitalWrite(pinControlD, LOW);
  bool joystickButtonState = digitalRead(joystickButtonPin);
  
  // Lire la valeur analogique de l'axe X
  int joystickX = analogRead(A0) - neutralX;
  
  // Activer le CD4066 pour lire l'axe Y (connecté à A0)
  digitalWrite(pinControlC, LOW);
  digitalWrite(pinControlD, HIGH);
  
  // Lire la valeur analogique de l'axe Y
  int joystickY = analogRead(A0) - neutralY;

  // Vérifier si les valeurs ont changé
  //abs(joystickX - prevJoystickX) > 10 || abs(joystickY - prevJoystickY) > 10;
  //const int comparaison = true ;

  if (true) {    
    // Afficher la valeur lue sur l'axe X
    //Serial.print("Joystick X : ");
    Serial.print(joystickX);
    Serial.print(",");
    
    // Afficher la valeur lue sur l'axe Y
    //Serial.print("Joystick Y : ");
    Serial.println(joystickY);
    
    // Mettre à jour les valeurs précédentes
    prevJoystickX = joystickX;
    prevJoystickY = joystickY;

    /////////////// 🛜 SERVER ///////////////
      unsigned long currentMillis = millis();
      
      if(currentMillis - previousMillis >= interval) {
        
        // Check WiFi connection status
        if ((WiFiMulti.run() == WL_CONNECTED)) {

          // Créer un objet JSON
          StaticJsonDocument<200> doc;
          doc["joystickX"] = joystickX;
          doc["joystickY"] = joystickY;
          doc["joystickButtonState"] = joystickButtonState;

          // Convertir l'objet JSON en chaîne JSON
          String jsonString;
          serializeJson(doc, jsonString);

          coordinates = httpPOSTRequest("http://192.168.4.1/coordinates", jsonString);
          //Serial.println("Coordinates: " + coordinates );
          
          // save the last HTTP GET Request
          previousMillis = currentMillis;
        }
        else {
          //Serial.println("WiFi Disconnected");
        }
      }
  } else{
    //Serial.print("Nope");
  }

  delay(100);  // Attendre un court instant

}

/////////////// FUNCTIONS ///////////////
//Permet d'envoyer de la data via une requete POST
  String httpPOSTRequest(const char* serverName, const String& body) {
    WiFiClient client;
    HTTPClient http;
      
    // Your IP address with path or Domain name with URL path 
    http.begin(client, serverName);
    
    // Send HTTP POST request
    int httpResponseCode = http.POST(body); // Laissez le corps de la requête vide pour l'instant
    
    String payload = "--"; 
    
    if (httpResponseCode>0) {
      //Serial.print("HTTP Response code: ");
      //Serial.println(httpResponseCode);
      payload = http.getString();
    }
    else {
      //Serial.print("Error code: ");
      //Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();

    return payload;
  }
