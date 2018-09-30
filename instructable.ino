/****************************************

Date création             : 27/09/2018
Date mise en prod         : 27/09/2018
Version IDE Arduino       : 1.8.6
Upload speed              : 921600
Type de carte dans l'IDE  : "LOLIN(WEMOS) D1 R2 & mini"
Carte physique employée   : LOLIN(WEMOS) D1 R2 & mini (https://www.amazon.fr/gp/product/B01ELFAF1S/ref=oh_aui_detailpage_o00_s00?ie=UTF8&psc=1)


Pin   Function                      ESP-8266 Pin      Utilisation locale
---------------------------------------------------------------------------------------------
TX    TXD                           TXD
RX    RXD                           RXD
A0    Analog input, max 3.3V input  A0                Tension d'alimentaion
D0    IO                            GPIO16            Connecté à RST (pour le deep.sleep)
D1    IO, SCL                       GPIO5             
D2    IO, SDA                       GPIO4             
D3    IO,10k Pull-up                GPIO0             
D4    IO, 10k pull-up, BUILTIN_LED  GPIO2             Servo moteur
D5    IO, SCK                       GPIO14            Reed relève
D6    IO, MISO                      GPIO12            Reed lettre
D7    IO, MOSI                      GPIO13            Reed colis
D8    IO,10k pull-down, SS          GPIO15            
G     Ground                        GND
5V    5V                            –
3V3   3.3V                          3.3V
RST   Reset                         RST               Connecté à D0  (pour le deep.sleep)

****************************************/

#include <ESP8266WiFi.h>
#include <time.h>

  bool Logs       = true;

// wifi
    const char* ssid        = "XXX000";
    const char* password    = "XXX000";
 
    IPAddress ip(XXX, XXX, XXX, XXX);
    IPAddress dns(XXX, XXX, XXX, XXX);
    IPAddress gateway(XXX, XXX, XXX, XXX);
    IPAddress subnet(XXX, XXX, XXX, XXX);
    WiFiClient client;

// Servo
    #include <Servo.h>
    #define PIN_SERVO D4
    Servo myservo;

// Reeds
    #define PIN_SWITCH_OUT D5
    byte Old_Switch_State_OUT;
    byte Switch_State_OUT;
    #define PIN_SWITCH_IN_LETTER D6
    byte Old_Switch_State_IN_LETTER;
    byte Switch_State_IN_LETTER;
    #define PIN_SWITCH_IN_PARCEL D7
    byte Old_Switch_State_IN_PARCEL;
    byte Switch_State_IN_PARCEL;

//  Analog
    #define PIN_ANALOG A0

    unsigned long switchPressTime;
    const unsigned long DEBOUCE_TIME = 200;

// MQTT
    #include <PubSubClient.h>
    const char* MQTT_Server_IP  = "XXX, XXX, XXX, XXX";
    const int MQTT_Server_Port  = XXX;
    int IDX_Letter_Box          = XXX;
    PubSubClient ClientMQTT(client);
    char MQTT_Message_Buff[70];
    String MQTT_Pub_String;

// Tension
    float vcc = analogRead(PIN_ANALOG);

// NTP
    time_t tnow;
    int Old_Time = 0;
    int Int_Heures = 0;
    int Int_Minutes = 0;

void setup(){
    Serial.begin(115200);

    network(true);

      pinMode(PIN_SWITCH_OUT, INPUT_PULLUP); 
      Old_Switch_State_OUT = digitalRead (PIN_SWITCH_OUT);
      pinMode(PIN_SWITCH_IN_LETTER, INPUT_PULLUP); 
      Old_Switch_State_IN_LETTER = digitalRead (PIN_SWITCH_IN_LETTER);
      pinMode(PIN_SWITCH_IN_PARCEL, INPUT_PULLUP); 
      Old_Switch_State_IN_PARCEL = digitalRead (PIN_SWITCH_IN_PARCEL); 
    
    network(false);
}

void loop() {

  // NTP set
      tnow = time(nullptr);
      Int_Heures = String(ctime(&tnow)).substring(11,13).toInt();
      Int_Minutes = String(ctime(&tnow)).substring(14,16).toInt();

  // Mise en veille prolongée le soir dés 21h - pas la peine d'aller plus loin
      if(!((Int_Heures >= 8) && (Int_Heures <= 20))){
          Serial.print("Sleep pour la nuit (");
          Serial.print(60 - Int_Minutes);
          Serial.println(" minutes)");
          sleep(60 - Int_Minutes);        
      }
            
  // Reeds managment 
      Switch_State_OUT = digitalRead (PIN_SWITCH_OUT);
      if (Switch_State_OUT != Old_Switch_State_OUT){
          if (millis () - switchPressTime >= DEBOUCE_TIME){
              switchPressTime = millis ();
              if (Switch_State_OUT == HIGH){
                  Serial.println ("courrier relevé !");
                  MQTT_Pubilsh(IDX_Letter_Box, 0, "0");
              }   
          }
          Old_Switch_State_OUT = Switch_State_OUT;
      }

      Switch_State_IN_LETTER = digitalRead (PIN_SWITCH_IN_LETTER);
      if (Switch_State_IN_LETTER != Old_Switch_State_IN_LETTER){
          if (millis () - switchPressTime >= DEBOUCE_TIME){
              switchPressTime = millis ();
              if (Switch_State_IN_LETTER == HIGH){
                  Serial.println ("courrier arrivé !");
                  MQTT_Pubilsh(IDX_Letter_Box, 1, "Courrier");
              }
          }
          Old_Switch_State_IN_LETTER = Switch_State_IN_LETTER;
      }

      Switch_State_IN_PARCEL = digitalRead (PIN_SWITCH_IN_PARCEL);
      if (Switch_State_IN_PARCEL != Old_Switch_State_IN_PARCEL){
          if (millis () - switchPressTime >= DEBOUCE_TIME){
              switchPressTime = millis ();
              if (Switch_State_IN_PARCEL == HIGH){
                  Serial.println ("colis arrivé !");
                  MQTT_Pubilsh(IDX_Letter_Box, 1, "Colis");
              } 
          }
          Old_Switch_State_IN_PARCEL = Switch_State_IN_PARCEL;
      }

      // Servo management
      if (Old_Time != Int_Heures){
          Old_Time = Int_Heures;
          myservo.attach(PIN_SERVO);

          switch (Int_Heures) {
              case 7: 
                  if (Logs) Serial.println ("Positionne le servo pour 7 Heure");
                  myservo.write(180);
                  break;
              case 8:
                  if (Logs) Serial.println ("Positionne le servo pour 8 Heure");
                  myservo.write(170);
                  break;
              case 9:
                  if (Logs) Serial.println ("Positionne le servo pour 9 Heure");
                  myservo.write(160);
                  break;
              case 10:
                  if (Logs) Serial.println ("Positionne le servo pour 10 Heure");
                  myservo.write(150);
                  break;
              case 11:
                  if (Logs) Serial.println ("Positionne le servo pour 11 Heure");
                  myservo.write(135);
                  break;
              case 12:
                  if (Logs) Serial.println ("Positionne le servo pour 12 Heure");
                  myservo.write(120);
                  break;
              case 13:
                  if (Logs) Serial.println ("Positionne le servo pour 13 Heure");
                  myservo.write(102);
                  break;
              case 14:
                  if (Logs) Serial.println ("Positionne le servo pour 14 Heure");
                  myservo.write(82);
                  break;
              case 15:
                  if (Logs) Serial.println ("Positionne le servo pour 15 Heure");
                  myservo.write(63);
                  break;
              case 16:
                  if (Logs) Serial.println ("Positionne le servo pour 16 Heure");
                  myservo.write(43);
                  break;
              case 17:
                  if (Logs) Serial.println ("Positionne le servo pour 17 Heure");
                  myservo.write(30);
                  break;
              case 18:
                  if (Logs) Serial.println ("Positionne le servo pour 18 Heure");
                  myservo.write(15);
                  break;
              case 19:
                  if (Logs) Serial.println ("Positionne le servo pour 19 Heure");
                  myservo.write(1);
                  break;
              case 20:
                  if (Logs) Serial.println ("Positionne le servo pour 20 Heure");
                  myservo.write(1);
                  break;
              case 21: 
                  if (Logs) Serial.println ("Positionne le servo pour 21 Heure");
                  myservo.write(1);
                  break;
              case 22: 
                  if (Logs) Serial.println ("Positionne le servo pour 22 Heure");
                  myservo.write(1);
                  break;
          }
          delay(4000);
          myservo.detach();
          digitalWrite(PIN_SERVO, LOW);
       }
  
  // Mise en veille prolongée si dimanche
      if(String(ctime(&tnow)).substring(0,3) == "Sun"){
          Serial.print("Sleep pour le dimanche (");
          Serial.print(60 - Int_Minutes);
          Serial.println(" minutes)");
          sleep(60 - Int_Minutes); 
      }
  
  // Mise en veille prolongée si samedi aprés 13h
      if((String(ctime(&tnow)).substring(0,3) == "Sat") && (Int_Heures >= 13)){
          Serial.print("Sleep pour le samedi aprés midi (");
          Serial.print(60 - Int_Minutes);
          Serial.println(" minutes)");
          sleep(60 - Int_Minutes); 
      }
}


      void sleep(int Min_Duration){
        ESP.deepSleep(Min_Duration * 60e6);
      }

      void network(bool UpDown){
        if (UpDown){
              Serial.print("Active les éléments réseau ");
              WiFi.forceSleepWake();
              delay(1);
              
           // init WIFI
              WiFi.config(ip, dns, gateway, subnet);
              WiFi.begin(ssid, password);
        
              while (WiFi.status() != WL_CONNECTED) {
                  delay(500);
                  Serial.print(".");
              }
              delay(5000);
              Serial.println(".");
              Serial.print("\tConnecté - Address IP : ");
              Serial.println(WiFi.localIP());
        
           // init MQTT
              ClientMQTT.setServer(MQTT_Server_IP, MQTT_Server_Port);
        
           // Init NTP
              Serial.print("\tSynchro. horaire ");
              configTime(0, 0, "fr.pool.ntp.org");  
              setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 0);
              while(time(nullptr) <= 100000) {
                Serial.print(".");
                delay(100);
              }
              Serial.println(".");
        }
        else{
            Serial.println("Désactive les éléments réseau.");
            WiFi.disconnect();
            WiFi.mode( WIFI_OFF );
            WiFi.forceSleepBegin();
            delay(1);
        }
      }

      void callback(char* topic, byte* payload, unsigned int length) {
       }

      void reconnect() {
          while (!ClientMQTT.connected()) {
              Serial.print("Attempting MQTT connection...");
              // Attempt to connect
              if (ClientMQTT.connect("ESP8266ClientBAL")) {
                  Serial.println("connected");
              } else {
                  Serial.print("failed, rc=");
                  Serial.print(ClientMQTT.state());
                  Serial.println(" try again in 5 seconds");
                  // Wait 5 seconds before retrying
                  delay(5000);
              }
          }
      }

      void MQTT_Pubilsh(int Int_IDX, int N_Value, String S_Value) {
         network(true);
         delay(5000);
         
         if (!ClientMQTT.connected()) reconnect();
            vcc = analogRead(PIN_ANALOG)/10.24;
            Serial.println("Envoit des infos au serveur MQTT ...");
            MQTT_Pub_String = "{ \"idx\" : " + String(Int_IDX) + ", \"Battery\" : " + String(vcc,0) + ", \"nvalue\" : " + N_Value + ", \"svalue\" : \"" + S_Value + "\"}";
            MQTT_Pub_String.toCharArray(MQTT_Message_Buff, MQTT_Pub_String.length()+1);
            ClientMQTT.publish("domoticz/in", MQTT_Message_Buff);
          ClientMQTT.disconnect();
          
          delay(5000);
          network(false);
      }
