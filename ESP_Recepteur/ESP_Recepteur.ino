//Integration de librairie externes
#include <ESP8266WiFi.h>
#include "ESPAsyncWebServer.h"

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

#include <Servo.h>
#include <ArduinoJson.h>

#include <math.h>

///// 🛜 WIFI (variables) /////
  // Set your access point network credentials
  const char* ssid = "ESP8266-Access-Point";
  const char* password = "123456789";

  // Create AsyncWebServer object on port 80
  AsyncWebServer server(80);

  //Creation de la page Web
  const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP8266 Web Server</title>
    <style>
      .button {
        background-color: #4CAF50;
        border: none;
        color: white;
        padding: 15px 32px;
        text-align: center;
        text-decoration: none;
        display: inline-block;
        font-size: 16px;
        margin: 4px 2px;
        cursor: pointer;
      }
    </style>
  </head>
  <body>
    <h2>ESP8266 Web Server</h2>
    <button class="button" onclick="sendRequest('button1')">Start Moteur Phone</button>
    <button class="button" onclick="stopCamera('button2')">Stop Camera Moteur</button>

    <script>
      function sendRequest(button) {
        fetch('/coordinates', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({ button: button })
        })
        .then(response => {
          if (response.ok) {
            console.log('Requête envoyée avec succès.');
          } else {
            console.error('Erreur lors de l\'envoi de la requête.');
          }
        })
        .catch(error => {
          console.error('Erreur:', error);
        });
      }

      function stopCamera(button) {
        fetch('/stopCamera', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({ button: button })
        })
        .then(response => {
          if (response.ok) {
            console.log('Requête envoyée avec succès.');
          } else {
            console.error('Erreur lors de l\'envoi de la requête.');
          }
        })
        .catch(error => {
          console.error('Erreur:', error);
        });
      }
    </script>
  </body>
  </html>
  )rawliteral";

///// 🏎️ MOTEURS (variables) /////
  // Broches de contrôle du moteur connectées au module L298N
  const int IN1 = D1;
  const int IN2 = D2;
  const int IN6 = D6;
  const int IN7 = D7;

  Servo myservo;  // create servo object to control a servo

///// 💡LED (variables) //////
  const uint16_t PixelCount = 33; // make sure to set this to the number of pixels in your strip
  const uint16_t PixelPin = 3;  // make sure to set this to the correct pin, ignored for Esp8266
  const uint16_t AnimCount = 1; // we only need one
  const uint16_t TailLength = 6; // length of the tail, must be shorter than PixelCount
  const float MaxLightness = 0.2f; // max lightness at the head of the tail (0.5f is full bright)

  NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma

  NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(PixelCount, PixelPin);
  // for esp8266 omit the pin
  //NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(PixelCount);

  NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object


void setup(){
  // Serial port for debugging purposes
  Serial.begin(74880);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);  // Allumer la led bleu de l'ESP8266

  ///// 💡LED /////
    strip.Begin();
    strip.Show();

    SetRandomSeed();

    // Draw the tail that will be rotated through all the rest of the pixels
    DrawTailPixels();

    // we use the index 0 animation to time how often we rotate all the pixels
    animations.StartAnimation(0, 66, LoopAnimUpdate);

  
  ///// 🏎️ MOTEURS /////
    // Configuration des broches comme sortie
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN6, OUTPUT);
    pinMode(IN7, OUTPUT);

    //Servo moteur
    myservo.attach(14);

  ///// 🛜 WIFI /////
    // Setting the ESP as an access point
    Serial.print("Setting AP (Access Point)…");
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    //Requetes serveurs

    //Initialise la page web
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
    });
    
    //Permet de faire tourner les moteurs
    server.on("/coordinates", HTTP_POST, [](AsyncWebServerRequest *request){
      request->send(200);
    }, NULL, [] (AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Analyser les données JSON
      DynamicJsonDocument doc(1024);
      
      DeserializationError error = deserializeJson(doc, data);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }

      // Extraire les valeurs de joystickX et joystickY
      int joystickX = doc["joystickX"];
      int joystickY = doc["joystickY"];
      bool joystickButtonState = doc["joystickButtonState"];

      Serial.println(joystickButtonState);

      //Serial.println(joystickX);   

      /*
      //Print les infos reçus
       for (size_t i = 0; i < len; i++) {
        Serial.write(data[i]);
      } */

      EspLed(); //Fonctions pour faire clignoter la led Bleu de l'ESP recepteur lorsqu"elle des infos


      static int pos = 90; //Position initial du servomoteur

      //Check si le joystick est cliqué. Si on clique sur le boutton cela actionne le moteur pour faire tourner l'Iphone
      if (joystickButtonState){
        if (joystickX >= -20 && joystickX <= 20 && joystickY >= -20 && joystickY <= 20 ) {
          arreterMoteur();
          arreterCamera();
        } else {
          moteurHoraire();

          static unsigned long previousMillis = 0;
          const unsigned long interval = 100; // Intervalle entre chaque déplacement du servo moteur (en millisecondes)

          unsigned long currentMillis = millis();

          if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;

            if ( joystickX >= -300 && joystickX <= 300 ){
              myservo.write(90);
            } else if(joystickX <= -300) {
              myservo.write(0);
            } else if(joystickX >= 300){
              myservo.write(180);
            }

          }
        }
      } else{
        moteurCamera();      
      }
        
    });

    //Permet de stopper la rotation du moteur pour l'Iphone via la page web
    server.on("/stopCamera", HTTP_POST, [](AsyncWebServerRequest *request){
      request->send(200);
    }, NULL, [] (AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      arreterCamera();
    });


    // Start server
    server.begin();
}
 
void loop(){
  ///// 💡LED//////
    // this is all that is needed to keep it running
    // and avoiding using delay() is always a good thing for
    // any timing related routines
    animations.UpdateAnimations();
    strip.Show(); 
}


/////////// FONCTIONS ////////////
// 🏎️ MOTEURS (fonctions) //
  // Fonction pour faire tourner le moteur dans le sens horaire
  void moteurHoraire() {
    analogWrite(IN1, 255);
    digitalWrite(IN2, LOW);

    Serial.println("Moteur tourne dans le sens horaire");
  }

  // Fonction pour arrêter le moteur
  void arreterMoteur() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    Serial.println("Moteur arrêté");
  }

  // Fonction pour faire tourner le moteur dans le sens antihoraire
  void moteurAntihoraire() {
    analogWrite(IN1, 255);
    digitalWrite(IN2, HIGH);

    Serial.println("Moteur tourne dans le sens antihoraire");
  }

  void moteurCamera() {
    analogWrite(IN6, 20);
    //digitalWrite(IN6, HIGH);
    digitalWrite(IN7, LOW);

    Serial.println("Camera qui tourne");
  }

  void arreterCamera() {
    digitalWrite(IN6, LOW);
    digitalWrite(IN7, LOW);

    Serial.println("Camera arrêtée");
  }

 

// 💡LED (fonctions) //
  void EspLed(){
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    delay(100);                      // Wait for a second
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(100); 
  }

  void SetRandomSeed()
  {
      uint32_t seed;

      // random works best with a seed that can use 31 bits
      // analogRead on a unconnected pin tends toward less than four bits
      seed = analogRead(0);
      delay(1);

      for (int shifts = 3; shifts < 31; shifts += 3)
      {
          seed ^= analogRead(0) << shifts;
          delay(1);
      }

      // Serial.println(seed);
      randomSeed(seed);
  }

  void LoopAnimUpdate(const AnimationParam& param)
  {
      // wait for this animation to complete,
      // we are using it as a timer of sorts
      if (param.state == AnimationState_Completed)
      {
          // done, time to restart this position tracking animation/timer
          animations.RestartAnimation(param.index);

          // rotate the complete strip one pixel to the right on every update
          strip.RotateRight(1);
      }
  }

  void DrawTailPixels()
  {
      // using Hsl as it makes it easy to pick from similiar saturated colors
      float hue = random(360) / 360.0f;
      for (uint16_t index = 0; index < strip.PixelCount() && index <= TailLength; index++)
      {
          float lightness = index * MaxLightness / TailLength;
          RgbColor color = HslColor(hue, 1.0f, lightness);

          strip.SetPixelColor(index, colorGamma.Correct(color));
      }
  }



