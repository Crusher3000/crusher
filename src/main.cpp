#include <Arduino.h>
#include <Wire.h>//OLED
#include <Adafruit_GFX.h>//OLED
#include <Adafruit_SSD1306.h>//OLED
#include <WiFi.h>                                                               // Librairie Wi-Fi
#include <PubSubClient.h>  
#include <NewPing.h>
#include <Adafruit_NeoPixel.h>

TaskHandle_t Task1;
TaskHandle_t Task2;

const char* ssid = "AntoDB";                                                   // SSID du réseau Wi-Fi auquel il faut se connecter
const char* password = "3049oQ7%";                                           // Mot de passe du réseau Wi-Fi auquel il faut se connecter

#define PIN 19
#define NUMPIXELS 24
int pix;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int red = 0;                                                      // Valeur de la couleur rouge du ruban led
int green = 0;                                                    // Valeur de la couleur verte du ruban led
int blue = 0;                                                     // Valeur de la couleur bleue du ruban led

int i;
int j;
int k;

char str_battery[5];
int battery;
int battery_map;

const char* mqtt_server = "192.168.137.189"; 
int buzzer;
int turn;
int drive;
int stopCrusher;

int distance;
char chardistance[5];
     
#define TRIGGER_PIN  18
#define ECHO_PIN     5
#define MAX_DISTANCE 200
     
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

char dir[6];
char chardrive[3];
char charturn[3];
char charstopCrusher[3];

WiFiClient espClient;                                                           // Création de "espClient" propre au client Wi-Fi
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE  (50)                                                   // Défini la taille maximum des messages que l'on va envoyer
char msg[MSG_BUFFER_SIZE]; 

//oled
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
int OLEDclock;
int oldOLEDclock;
long second;
char charsecond[6];
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)

//BUZZER
int PinBuzzer = 17;
int lastbuzzer;
int HeureAppuiBP;
//LDR
int PinLDR = 12;
char charLDR[6];

int DirMotor;
int DirMotorPrecedent;

//StepperMotor
int DIR_RearStepperMotor = 0;
int STEP_RearStepperMotor = 2;
int DIR_FrontStepperMotor = 16;
int STEP_FrontStepperMotor = 4;
int StepperClock;
int DirClock;

//moteur traction
int IN1_Motor = 25;
int IN2_Motor = 33;

//tachymetre
int PinTachymetre = 14; 
int EtatTachymetre;
int ADCEtatTachymetre;
int EtatTachymetrePecedent;
long HeureActuelle;
long HeureDuDebutDuTour;
long HeureDuFinDuTour;
long TempsPourUnTour;
float TourMinute;
float KmHeure;
char charKmHeure[6];

//Relay
int Relay1 = 23;

//Limit Switch
int LiSw1 = 26;
int LiSw2 = 27;
int LiSw3 = 13;
int LiSw4 = 15;

int LiSw1Value;
int LiSw2Value;
int LiSw3Value;
int LiSw4Value;

char LiSw1Char[5];
char LiSw2Char[5];
char LiSw3Char[5];
char LiSw4Char[5];

void setup_wifi() {                                                             // Fonction de connection au WiFi

  delay(10);                                                                    // Délai de 10ms
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);                                                         // Imprime dans la console le nom du WiFi

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);                                                   // Lancement de la connexion WiFi

  while (WiFi.status() != WL_CONNECTED) {                                       // Tant que le microcontrôleur n'est pas connecté au WiFi
    delay(500);                                                                 // Délai de 500ms
    Serial.print(".");                                                          // Imprimme un point
  }

  randomSeed(micros());                                                         // Création d'une "clef" aléatoire

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());                                               // Affichage dans la console que l'on est connecté avec cette adresse IP
}

void callback(char* topic, byte* payload, unsigned int length) {                // Fonction de Callback quand un message MQTT arrive sur un topic (subscribe)
  Serial.print("Message arrived [");
  Serial.print(topic);                                                          // On affiche de quel topic il s'agit
  Serial.print("] ");
  for (int i = 0; i < length; i++) {                                            // On boucle tous les caractères reçus sur le topic
    Serial.print((char)payload[i]);                                             // On affiche un à un tous les caractères reçus
  }
  Serial.println();
  
  char buffer1[length+1];                                                       // On crée une variable local de buffer
  for (int i = 0; i < length+1; i++) {                                          // On relis tous les caractères reçus sur le topic
    buffer1[i] = (char)payload[i];                                              // On enregistre tous les caractères que l'on a besoin (uniquement)
  }
  if (String(topic) == "buzzer") {                                             // On vérifie si c'est le bon topic
    buzzer = atoi(buffer1);           
  }
  if (String(topic) == "drive") {//Marche arriere/avant                                             // On vérifie si c'est le bon topic
    drive = atoi(buffer1);           
  }
  if (String(topic) == "turn") { //droite/gauche                                            // On vérifie si c'est le bon topic
    turn = atoi(buffer1);           
  }
  if (String(topic) == "stopCrusher") { //droite/gauche                                            // On vérifie si c'est le bon topic
    stopCrusher = atoi(buffer1);           
  }
  if (String(topic) == "led_red") {                               // On vérifie si c'est le bon topic
    red = atoi(buffer1);                                          // Alors on le stocke dans la variable correspondante
  }
  if (String(topic) == "led_green") {                             // On vérifie si c'est le bon topic
    green = atoi(buffer1);                                        // Alors on le stocke dans la variable correspondante
  }
  if (String(topic) == "led_blue") {                              // On vérifie si c'est le bon topic
    blue = atoi(buffer1);                                         // Alors on le stocke dans la variable correspondante
  }
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {                                                 // Si le client se connecte
    Serial.print("Attempting MQTT connection...");                              // On attent la conexion MQTT
    if (client.connect("ESP32")) {                                              // Si le client se connecte en étant appelé "ESP32"
      Serial.println("connected");                                              // On prévient qu'il est connecté
      client.subscribe("drive");   
      client.subscribe("turn");// On capture la donnée du topic "inTopic"
      client.subscribe("buzzer");
      client.subscribe("led_red");                                              // On capture la donnée du topic "led_red"
      client.subscribe("led_green");                                            // On capture la donnée du topic "led_green"
      client.subscribe("led_blue");                                             // On capture la donnée du topic "led_blue"
    } else {                                                                    // Si la connexion rate
      Serial.print("failed, rc=");                                              // On affiche qu'il y a une erreur
      Serial.print(client.state());                                             // On affiche le numéro de l'erreur
      Serial.println(" try again in 5 seconds");                                // On affiche comme quoi on réessaye
      delay(5000);                                                              // On attend 5 secondes avant de réessayer
    }
  }
}

void printToOLED(int x, int y,  char *message){
  display.setCursor(x, y); 
  //display.setTextColor(WHITE,BLACK); //Superposer les texte
  display.print(message);
  display.display();
}

void setup() {
  Serial.begin(115200); 
  //Relay
  pinMode(Relay1, OUTPUT);
  digitalWrite(Relay1, LOW);
  
  
  // LEDs
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  pixels.setBrightness(200);
  
  client.setServer(mqtt_server, 1883);                                          // On se connecte au serveur MQTT
  client.setCallback(callback);  
  
    //3V3
  //pinMode(32, OUTPUT);
  //digitalWrite(32,HIGH);
  setup_wifi();
  
  //StepperMotor
  pinMode(DIR_RearStepperMotor, OUTPUT);
  pinMode(STEP_RearStepperMotor, OUTPUT);
  pinMode(DIR_FrontStepperMotor, OUTPUT);
  pinMode(STEP_FrontStepperMotor, OUTPUT);
  
  //OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.drawRoundRect(0, 0, 90, 30, 3, WHITE); //x1, y1, largeur, hauteur, rayon
  display.drawRoundRect(94, 0, 33, 64, 3, WHITE); //x1, y1, largeur, hauteur, rayon
  display.drawRoundRect(0, 34, 90, 30, 3, WHITE); //x1, y1, largeur, hauteur, rayon
  display.setTextSize(1);  
  display.setTextColor(WHITE, BLACK);  
  printToOLED(5, 5, "Speed :");
  printToOLED(5, 17,"Lum :");
  printToOLED(5, 40, "Batterie :");
  printToOLED(5, 52, "Dist :");

  pinMode(PinBuzzer, OUTPUT);
  //BUZZER


  //moteur traction
  pinMode(IN1_Motor, OUTPUT);
  pinMode(IN2_Motor, OUTPUT);

  //Tachymetre
  pinMode(PinTachymetre, INPUT);

  xTaskCreatePinnedToCore(Task1code,"Task1",10000,NULL,1,&Task1,0);
  delay(500); 

  xTaskCreatePinnedToCore(Task2code,"Task2",10000,NULL,1,&Task2,1);
  delay(500); 
}

void Task1code( void * parameter ){
  Serial.print("Task1 is running on core ");
  Serial.println(xPortGetCoreID());
  const TickType_t xDelayTask2 = 1 / portTICK_PERIOD_MS ;
  for(;;){
  if (!client.connected()) {                                                    // Si le client pour le MQTT en WiFi n'est pas connecté
    reconnect();                                                                // On appelle la fonction qui demande une reconnexion
  }
  client.loop();
  
  vTaskDelay( xDelayTask2 );
  digitalWrite(STEP_RearStepperMotor,StepperClock);
  digitalWrite(DIR_RearStepperMotor,DirClock);
  digitalWrite(STEP_FrontStepperMotor,StepperClock);
  digitalWrite(DIR_FrontStepperMotor,not(DirClock));

  //Tachymetre
  EtatTachymetre = digitalRead(PinTachymetre);

  HeureActuelle = millis();         
  if (EtatTachymetre == 1 & EtatTachymetrePecedent == 0){
    if (HeureDuDebutDuTour == 0){
      HeureDuDebutDuTour = HeureActuelle;
      TempsPourUnTour = HeureDuDebutDuTour - HeureDuFinDuTour;
      HeureDuFinDuTour = 0;
    }
    else {
      HeureDuFinDuTour = HeureActuelle;
      TempsPourUnTour = HeureDuFinDuTour - HeureDuDebutDuTour;
      HeureDuDebutDuTour = 0;
    }
    TourMinute = 1/((TempsPourUnTour /1000.00)/60.00);  
    KmHeure = (0.073 * 3.6 * 3.14 * TourMinute)/60.00;   
        
  }
  EtatTachymetrePecedent = EtatTachymetre;

    if(lastbuzzer == 0 && buzzer == 1) {
      HeureAppuiBP = millis();
      // toggle state of LED
      Serial.println("APPUI");
    }
    lastbuzzer  = buzzer; 
    
    if((HeureAppuiBP + 3000 <= millis()) and buzzer)
    {
      
      Serial.println("OK");
    }

      
  //BUZZER
  digitalWrite(PinBuzzer, buzzer); 

  //Limit Switch
  LiSw1Value = digitalRead(LiSw1);
  LiSw2Value = digitalRead(LiSw2);
  LiSw3Value = digitalRead(LiSw3);
  LiSw4Value = digitalRead(LiSw4);
  
  sprintf(chardrive, "%2u", drive);
  if (drive == 1 ){
    digitalWrite(IN1_Motor,LOW);
    digitalWrite(IN2_Motor,HIGH);
  }

  else if (drive == 2){
    digitalWrite(IN2_Motor,LOW);
    digitalWrite(IN1_Motor,HIGH);
  }
    
  else{
    digitalWrite(IN1_Motor,LOW);
    digitalWrite(IN2_Motor,LOW);
  }
  
  sprintf(charturn, "%2u", turn);
  
  if ((charturn[1] == '1' ) ){
    if (LiSw3Value == 0){
      StepperClock = (millis()/2)%2;
    }
    DirClock = 1;
    //Serial.println(charturn);
    charturn[1] = ' ';
    //dir[0] = 'D';
    
  }

  else if ((charturn[1] == '2') ){
    if (LiSw4Value == 0){
      StepperClock = (millis()/2)%2;
    }
    
    DirClock = 0;
    //Serial.println(charturn);
    charturn[1] = ' ';
    //dir[0] = 'G'; 
    
  }
    
  else{
    StepperClock = 0;
    //dir[0] = 'C';
   
  }
  //Serial.println(dir[0]);
  

//  Serial.print("Ping: ");
//  Serial.print(uS / US_ROUNDTRIP_CM);
//  Serial.println("cm");
  } 
}

void Task2code( void * parameter ){
  Serial.print("Task2 is running on core ");
  Serial.println(xPortGetCoreID());
  const TickType_t xDelayTask1 = 500 / portTICK_PERIOD_MS ;
  for(;;){ 

    client.publish("speed", charKmHeure);                          // On publie sur un topic l'humidité en chaîne de caractères
    client.publish("distance", chardistance);                          // On publie sur un topic l'humidité en chaîne de caractères
    int uS = sonar.ping();
    distance = uS / US_ROUNDTRIP_CM;
    vTaskDelay( xDelayTask1 );
    
    sprintf(chardistance, "%5u  ", distance);
    printToOLED(50, 52, chardistance);            

    //printToOLED(50, 5, chardrive);
    sprintf(charKmHeure, "%0.2f  ", KmHeure);
    printToOLED(50, 5, charKmHeure);
    //Serial.println(charKmHeure);

    sprintf(LiSw1Char, "%2u", LiSw1Value);
    printToOLED(100, 5, LiSw1Char);    
    sprintf(LiSw2Char, "%2u", LiSw2Value);
    printToOLED(100, 17, LiSw2Char);
    sprintf(LiSw3Char, "%2u", LiSw3Value);
    printToOLED(100, 40, LiSw3Char);
    sprintf(LiSw4Char, "%2u", LiSw4Value);
    printToOLED(100, 52, LiSw4Char);
    
    pix ++;
    if (pix%2 == 0){ 
      //Serial.println(pix);
       // Set all pixel colors to 'off'
      if ((charturn[1] == '1' ) ){
        for ( i = 0; i <= 2; i++){
          pixels.setPixelColor(i, 255, 100, 0);
        }
        for ( j = 9; j <= 11; j++){
          pixels.setPixelColor(j, 255, 100, 0);
        }
      }

      else if ((charturn[1] == '2') ){
        for ( int l = 12; l <= 14; l++){
          pixels.setPixelColor(j, 255, 100, 0);
        }
        for ( k = 21; k <= 23; k++){
          pixels.setPixelColor(k, 255, 100, 0);
        }
      }
    }
    else {
      for ( int z = 0; z <= 23; z++) {
        pixels.setPixelColor(z, red, green, blue);
      }
    }
    if (drive == 1 ) {
      for ( i = 0; i <= 2; i++){
        pixels.setPixelColor(i, 255, 255, 255);
      }
      for ( k = 21; k <= 23; k++){
        pixels.setPixelColor(k, 255, 255, 255);
      }
      for ( j = 9; j <= 11; j++){
        pixels.setPixelColor(j, 100, 0, 0);
      }
      for ( int l = 12; l <= 14; l++){
        pixels.setPixelColor(j, 100, 0, 0);
      }
    }
    else if (drive == 2) {
      for ( i = 0; i <= 2; i++){
        pixels.setPixelColor(i, 255, 255, 255);
      }
      for ( k = 21; k <= 23; k++){
        pixels.setPixelColor(k, 255, 255, 255);
      }
      for ( j = 9; j <= 11; j++){
        pixels.setPixelColor(j, 255, 0, 0);
      }
      for ( int l = 12; l <= 14; l++){
        pixels.setPixelColor(j, 255, 0, 0);
      }
    }
    pixels.show(); // Send the updated pixel colors to the hardware.   

    battery = analogRead(34);
    battery_map = map(battery, 2000,3700,0,100);
    sprintf(str_battery,"%4u",battery_map);
    printToOLED(50, 40, str_battery);
    printToOLED(75, 40, "%");
    //Serial.println(xPortGetCoreID());
  }
}

void loop() {

}