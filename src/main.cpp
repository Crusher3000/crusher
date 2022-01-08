#include <Arduino.h>
#include <Wire.h>                                                 // Bibliothèque I²C pour l'OLED
#include <Adafruit_GFX.h>                                         // Bibliothèque incluant des fonctions graphique pour l'OLED
#include <Adafruit_SSD1306.h>                                     // Bibliothèque reprenant l'ensenmbles des fonctions d'utilisation de l'OLED
#include <WiFi.h>                                                 // Bibliothèque permettant la gestion Wi-Fi de notre ESP32
#include <PubSubClient.h>                                         // Bibliothèque fournissant des fonctions de messageries MQTT
#include <NewPing.h>                                              // Bibliothèque incluant des fonctions de mesure de distance par un capteur ultrasonique

// TACHES
TaskHandle_t Task1;                                               // Creation d'un objet de classe Task1 de type FreeRTOS
TaskHandle_t Task2;                                               // Creation d'un objet de classe Task2 de type FreeRTOS

// RESEAUX & COMMUNNICATION
const char* ssid = "AntoDB";                                      // SSID du réseau Wi-Fi auquel il faut se connecter
const char* password = "3049oQ7%";                                // Mot de passe du réseau Wi-Fi auquel il faut se connecter
const char* mqtt_server = "192.168.137.189";                      // Adresse IP du server MQTT auquel il faut se connecter
WiFiClient espClient;                                             // Création de "espClient" propre au client Wi-Fi
PubSubClient client(espClient);                                   // Création d'un système de messagerie MQTT du client

// RECEPTION MQTT
int buzzer;                                                       // Tramme MQTT provenant de la manette (BIP/no BIP ; 1/0)
int turn;                                                     // Tramme MQTT provenant de la manette (Marche arriere/avant/arret ; 2/1/0)
int drive;                                                      // Tramme MQTT provenant de la manette (droite/gauche/arret ; 2/1/0)
char chardrive[3];                                              // Tramme formatée provenant de la manette (Marche arriere/avant/arret ; 2/1/0)
char charturn[3];                                             // Tramme formatée provenant de la manette (droite/gauche/arret ; 2/1/0)

// EMISSION MQTT
char chardistance[5];                                             // Tramme MQTT à envoyer correspondant à la distance mesurée depuis le capteur ultrasonique
char charKmHeure[6];                                              // Tramme MQTT à envoyer correspondant à la vitesse du véhicule en km/h

// CAPTEUR ULTRASONIQUE
int distance;                                                     // Distance en cm mesurée par le capteur ultrasonique
#define TRIGGER_PIN  18                                           // PIN TRIGGER du capteur ultrasonique
#define ECHO_PIN     5                                            // PIN ECHO du capteur ultrasonique
#define MAX_DISTANCE 200                                          // Distance maximale souhaitée en cm
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);               // Configuration & création de notre capteur ultrasonique "sonar"

// OLED
#define SCREEN_WIDTH 128                                          // Largeur en pixel de l'OLED
#define SCREEN_HEIGHT 64                                          // Hauteur en pixel de l'OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // Création de notre OLED SSD1306 pour communiquer avec l'I²C (SDA, SCL)

// BUZZER
int PinBuzzer = 17;                                               // PIN du buzzer
int lastbuzzer;                                                   // Variable de détection du front montant de la tramme recue
int HeureAppuiBP;                                                 // Variable utilisée pour compter le temps de bipage afin d'éteindre le véhicule après 3 seconde (d'appui sur le switch de la manette)

// LDR
int PinLDR = 12;                                                  // PIN de la LDR                                                      

// MOTEUR PAS A PAS
int DIR_RearStepperMotor = 0;                                     // PIN DIR du driver DRV8825 du stepper motor M3
int STEP_RearStepperMotor = 2;                                    // PIN STEP du driver DRV8825 du stepper motor M3
int DIR_FrontStepperMotor = 16;                                   // PIN DIR du driver DRV8825 du stepper motor M4
int STEP_FrontStepperMotor = 4;                                   // PIN STEP du driver DRV8825 du stepper motor M4
int StepperClock;                                                 // Horloge commune des deux stepper motor qui sera injecté dans la PIN STEP de chaque driver
int DirClock;                                                     // Variable indiquant la sens de rotation de chaque moteur pas à pas (0 ou 1)

// MOTEUR DE TRACTION
int IN1_Motor = 25;                                               // PIN IN1 & IN3 du driver LM298 des moteurs de tractions (0 ou 1)
int IN2_Motor = 33;                                               // PIN IN2 & IN4 du driver LM298 des moteurs de tractions (0 ou 1)

// TACHYMETRE
int PinTachymetre = 14;                                           // PIN du tachymetre
int EtatTachymetre;                                               // Variable correspondant à l'état du tachymetre (0 ou 1)
int EtatTachymetrePecedent;                                       // Variable correspondant à l'état précédent du tachymetre (0 ou 1) pour identifier les front montants
long HeureActuelle;                                               // Variable reprenant le temps depuis lequel l'esp est connecté   
long HeureDuDebutDuTour;                                          // Variable reprenant le moment/l'instant depuis lequel le tour d'une roue a débuté 
long HeureDuFinDuTour;                                            // Variable reprenant le moment/l'instant depuis lequel ce meme tour d'une roue a fini 
long TempsPourUnTour;                                             // Variable correspondant à la différence entre le temps du début et de fin de tour d'une roue
float TourMinute;                                                 // Variable correspondant à la valeure réelle en tr/min d'une roue
float KmHeure;                                                    // Variable correspondant à la valeure réelle en km/h

// RELAIS
int Relay1 = 23;                                                  // PIN du relais

// FIN DE COURSES
int LiSw1 = 26;                                                   // Pin du Fin de course droite du M3
int LiSw2 = 27;                                                   // Pin du Fin de course gauche du M3
int LiSw3 = 13;                                                   // Pin du Fin de course droite du M4
int LiSw4 = 15;                                                   // Pin du Fin de course gauche du M4

int LiSw1Value;                                                   // Valeurs de l'état du fin de course
int LiSw2Value;                                                   // Valeurs de l'état du fin de course
int LiSw3Value;                                                   // Valeurs de l'état du fin de course
int LiSw4Value;                                                   // Valeurs de l'état du fin de course

char LiSw1Char[5];                                                // Valeurs de l'état formaté du fin de course 
char LiSw2Char[5];                                                // Valeurs de l'état formaté du fin de course 
char LiSw3Char[5];                                                // Valeurs de l'état formaté du fin de course 
char LiSw4Char[5];                                                // Valeurs de l'état formaté du fin de course 

void setup_wifi() {                                               // Fonction de connection au WiFi

  delay(10);                                                      // Délai de 10ms
  Serial.println();                                               // Imprime un retour à la ligne dans la console
  Serial.print("Connecting to ");                                 // Imprime un message dans la console
  Serial.println(ssid);                                           // Imprime dans la console le nom du WiFi

  WiFi.mode(WIFI_STA);                                            // Execution du type de mode de la connexion WiFi
  WiFi.begin(ssid, password);                                     // Lancement de la connexion WiFi

  while (WiFi.status() != WL_CONNECTED) {                         // Tant que le microcontrôleur n'est pas connecté au WiFi
    delay(500);                                                   // Délai de 500ms
    Serial.print(".");                                            // Imprimme un point dans la console
  }

  randomSeed(micros());                                           // Création d'une "clef" aléatoire

  Serial.println("");                                             // Imprime un message dans la console
  Serial.println("WiFi connected");                               // Imprime un message dans la console
  Serial.println("IP address: ");                                 // Imprime un message dans la console
  Serial.println(WiFi.localIP());                                 // Affichage dans la console que l'on est connecté avec cette adresse IP
}

void callback(char* topic, byte* payload, unsigned int length) {  // Fonction de Callback quand un message MQTT arrive sur un topic (subscribe)
  Serial.print("Message arrived [");                              // Imprime un message dans la console
  Serial.print(topic);                                            // On affiche de quel topic il s'agit
  Serial.print("] ");                                             // Imprime un message dans la console
  for (int i = 0; i < length; i++) {                              // On boucle tous les caractères reçus sur le topic
    Serial.print((char)payload[i]);                               // On affiche un à un tous les caractères reçus
  }
  Serial.println();                                               // Imprime un retour à la ligne dans la console
  
  char buffer1[length+1];                                         // On crée une variable local de buffer
  for (int i = 0; i < length+1; i++) {                            // On relis tous les caractères reçus sur le topic
    buffer1[i] = (char)payload[i];                                // On enregistre tous les caractères que l'on a besoin (uniquement)
  }
  if (String(topic) == "buzzer") {                                // On vérifie si c'est le bon topic
    buzzer = atoi(buffer1);                                       // Alors on le stocke dans la variable correspondante
  }
  if (String(topic) == "drive") {                               // On vérifie si c'est le bon topic (Marche arriere/avant/stop)
    drive = atoi(buffer1);                                      // Alors on le stocke dans la variable correspondante
  }
  if (String(topic) == "turn") {                              // On vérifie si c'est le bon topic (droite/gauche/arret)
    turn = atoi(buffer1);                                     // Alors on le stocke dans la variable correspondante
  }
  
}

void reconnect() {                                                // Fonction effectuant des tentative de reconnexion 
  while (!client.connected()) {                                   // Si le client se connecte
    Serial.print("Attempting MQTT connection...");                // On attent la conexion MQTT
    if (client.connect("ESP32")) {                                // Si le client se connecte en étant appelé "ESP32"
      Serial.println("connected");                                // On prévient qu'il est connecté
      client.subscribe("drive");                                // On capture une des données du topic
      client.subscribe("turn");                               // On capture une des données du topic
      client.subscribe("buzzer");                                 // On capture une des données du topic
    } 
    else {                                                        // Si la connexion rate
      Serial.print("failed, rc=");                                // On affiche qu'il y a une erreur
      Serial.print(client.state());                               // On affiche le numéro de l'erreur
      Serial.println(" try again in 5 seconds");                  // On affiche comme quoi on réessaye
      delay(5000);                                                // On attend 5 secondes avant de réessayer
    }
  }
}

void printToOLED(int x, int y,  char *message){                   // Fonction affichant un message dans l'OLED à une cetraine position
  display.setCursor(x, y);                                        // On place le cursor du message aux coordonner X Y avant de l'afficher
  display.print(message);                                         // On place ce message sur ce cursor
  display.display();                                              // On affiche le tout
}

void setup() {          
  Serial.begin(115200);                                           // Début de communication série à 115200 bps
  // RELAY
  pinMode(Relay1, OUTPUT);                                        // Mis en sortie du relais
  digitalWrite(Relay1, LOW);                                      // Au démarrage de l'esp on allume le relais pour maintenir le système en marche

  // COMMUNICATION
  client.setServer(mqtt_server, 1883);                            // On se connecte au serveur MQTT
  client.setCallback(callback);                                   // On synchronise aux messages entrant en MQTT 
  setup_wifi();                                                   // Démarrage de la communication Wi-Fi
  
  // MOTEUR PAS A PAS
  pinMode(DIR_RearStepperMotor, OUTPUT);                          // Mise en sortie de la PIN DIR de M3
  pinMode(STEP_RearStepperMotor, OUTPUT);                         // Mise en sortie de la PIN STEP de M3
  pinMode(DIR_FrontStepperMotor, OUTPUT);                         // Mise en sortie de la PIN DIR de M4
  pinMode(STEP_FrontStepperMotor, OUTPUT);                        // Mise en sortie de la PIN STEP de M4
  
  // OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {                // Initialisation de l'OLED avec son adresse I²C
    Serial.println(F("SSD1306 allocation failed"));               // Affichage d'une erreur dans la console si l'initialisation a échouée
    for(;;);                                                      // Arret du programme dans une boucle infinie
  }
  delay(2000);                                                    // Delai
  display.clearDisplay();                                         // Nettoyage de tous les éléments graphics présents dans l'OLED
  display.drawRoundRect(0, 0, 90, 30, 3, WHITE);                  // Creation d'un rectangle (x1, y1, largeur, hauteur, rayon)
  display.drawRoundRect(94, 0, 33, 64, 3, WHITE);                 // Creation d'un rectangle (x1, y1, largeur, hauteur, rayon)
  display.drawRoundRect(0, 34, 90, 30, 3, WHITE);                 // Creation d'un rectangle (x1, y1, largeur, hauteur, rayon)
  display.setTextSize(1);                                         // Configuration de la taille de police
  display.setTextColor(WHITE, BLACK);                             // Configuration d'écriture de blanc sur noir dans l'OLED
  printToOLED(5, 5, "Speed :");                                   // Affichage d'un message à l'OLED
  printToOLED(5, 17,"Lum :");                                     // Affichage d'un message à l'OLED
  printToOLED(5, 40, "Batterie :");                               // Affichage d'un message à l'OLED
  printToOLED(5, 52, "Dist :");                                   // Affichage d'un message à l'OLED

  // BUZZER  
  pinMode(PinBuzzer, OUTPUT);                                     // Mise en sortie de la pin du buzzer

  // MOTEUR DE TRACTION
  pinMode(IN1_Motor, OUTPUT);                                     // Mise en sortie de la pin IN1 & IN3 des moteurs
  pinMode(IN2_Motor, OUTPUT);                                     // Mise en sortie de la pin IN1 & IN3 des moteurs

  //Tachymetre
  pinMode(PinTachymetre, INPUT);                                  // Mise en entrée de la pin du tachymetre

  xTaskCreatePinnedToCore(Task1code,"Task1",10000,NULL,1,&Task1,0); // création de la tache 1 sous le coeur 0
  delay(500); 

  xTaskCreatePinnedToCore(Task2code,"Task2",10000,NULL,1,&Task2,1); // création de la tache 2 sous le coeur 1
  delay(500); 
}

void Task1code( void * parameter ){                               // Tache 1
  Serial.print("Task1 is running on core ");                      // Affichage d'un message dans la console
  Serial.println(xPortGetCoreID());                               // Affichage du coeur en plein execution dans la console
  const TickType_t xDelayTask2 = 1 / portTICK_PERIOD_MS ;         // Création d'un délais de 1 ms permettant de candencer la vitesse de la boucle suivante
  
  for(;;){                                                        // Début de la boucle 

    vTaskDelay( xDelayTask2 );                                    // Délai de 1 ms

    //COMMUNICATION
    if (!client.connected()) {                                    // Si le client pour le MQTT en WiFi n'est pas connecté
      reconnect();                                                // On appelle la fonction qui demande une reconnexion
    }
    client.loop();                                                // Synchronisation du noeud de l'esp à la boucle

    // TACHYMETRE
    EtatTachymetre = digitalRead(PinTachymetre);                  // Lecture de l'état du tachymetre
  
    HeureActuelle = millis();                                     // Récuperation du temps actuel afin de calculer la vitesse de la voiture
    if (EtatTachymetre == 1 & EtatTachymetrePecedent == 0){       // Lorsque le capteur du tachymètre vient de sortir de la bande noir de la jante de la roue
      if (HeureDuDebutDuTour == 0){                               // Si nous n'étions pas en début de tour cela signifie que nous avons fini le tour, et on commence ce calcul...
        HeureDuDebutDuTour = HeureActuelle;                       // On récupère l'instant dans lequel on est sorti de la bande noir en tant qu'heure de début d'un tout nouveau tour
        TempsPourUnTour = HeureDuDebutDuTour - HeureDuFinDuTour;  // On effectue la différence entre le moment où on avait commencé et fini le tour
        HeureDuFinDuTour = 0;                                     // On reset la valeur de la fin du tour
      }
      else {                                                      // Sinon, si nous étions en fin de tour...
        HeureDuFinDuTour = HeureActuelle;                         // On récupère l'instant dans lequel on est sorti de la bande noir en tant qu'heure de fin du tour
        TempsPourUnTour = HeureDuFinDuTour - HeureDuDebutDuTour;  // On effectue la différence entre le moment où on avait fini et commencé le tour
        HeureDuDebutDuTour = 0;                                   // On reset la valeur du début du tour
      }
      TourMinute = 1/((TempsPourUnTour /1000.00)/60.00);          // On calcule les tours/minute depuis la différence de temps calculé précédement
      KmHeure = (0.073 * 3.6 * 3.14 * TourMinute)/60.00;          // On converti les tours/minute en km/h 
          
    }
    EtatTachymetrePecedent = EtatTachymetre;                      // On reprend l'état du tachymètre comme celui précédent pour détecter les fronts montants dans le cycle suivant

    // RELAY (par le biais de la tramme buzzer)
    if(lastbuzzer == 0 && buzzer == 1) {                          // Detection du front montant de la tramme recue buzzer
      HeureAppuiBP = millis();                                    // On recupère l'instant où on a appuié sur le joystick
      Serial.println("APPUI");                                    // Affichage d'un message dans la console
    }
    lastbuzzer  = buzzer;                                         // On reprend l'état de la tramme comme celui précédent pour détecter les fronts montants dans le cycle suivant 
       
    if((HeureAppuiBP + 3000 <= millis()) and buzzer){             // Si cela fait plus de 3 seconde qu'il y a un BIP 
      digitalWrite(Relay1, HIGH);                                 // On étteint le relais
      Serial.println("OK");                                       // Affichage d'un message dans la console
    }
  
    // BUZZER
    digitalWrite(PinBuzzer, buzzer);                              // Bipage du buzzer lorsque la trame buzzer est à 1
  
    // FIN DE COURSe
    LiSw1Value = digitalRead(LiSw1);                              // Lecture de l'état de chaque fin de course
    LiSw2Value = digitalRead(LiSw2);                              // Lecture de l'état de chaque fin de course
    LiSw3Value = digitalRead(LiSw3);                              // Lecture de l'état de chaque fin de course
    LiSw4Value = digitalRead(LiSw4);                              // Lecture de l'état de chaque fin de course

    // MOTEURS DE TRACTION M1 & M2
    sprintf(chardrive, "%2u", drive);                         // Formattage de la tramme des moteur de traction
    if (chardrive[0] == '1' ){                                  // Si elle est à 1
      digitalWrite(IN2_Motor,LOW);                                // Activation de la marche avant
      digitalWrite(IN1_Motor,HIGH);                               // Activation de la marche avant
    }
  
    else if (chardrive[0] == '2'){                              // Si elle est à 2
      digitalWrite(IN1_Motor,LOW);                                // Activation de la marche arrière
      digitalWrite(IN2_Motor,HIGH);                               // Activation de la marche arrière
    }
      
    else{                                                         // Sinon, si 0
      digitalWrite(IN1_Motor,LOW);                                // Moteurs de tractions à l'arret
      digitalWrite(IN2_Motor,LOW);                                // Moteurs de tractions à l'arret
    }

    // MOTEURS PAS A PAS M3 & M4
    sprintf(charturn, "%2u", turn);                       // Formattage de la tramme des moteur pas à pas
    if (charturn[1] == '1' ){                                 // Si elle est à 1
      StepperClock = (millis()/2)%2;                              // Horloge de periode 2ms activé 
      DirClock = 1;                                               // Direction à 1 (droite)
    }
  
    else if (charturn[1] == '2'){                             // Si elle est à 2
      StepperClock = (millis()/2)%2;                              // Horloge de periode 2ms activé 
      DirClock = 0;                                               // Direction à 0 (droite)
    }
      
    else{                                                         // Sinon, si 0
      StepperClock = 0;                                           // Horloge désactivé
    }
  
    digitalWrite(STEP_RearStepperMotor,StepperClock);             // Ecriture des différents paramètres précédement traité vers les drivers (horloge [STEP] & Direction [DIR])  
    digitalWrite(DIR_RearStepperMotor,DirClock);                  // Ecriture des différents paramètres précédement traité vers les drivers (horloge [STEP] & Direction [DIR])  
    digitalWrite(STEP_FrontStepperMotor,StepperClock);            // Ecriture des différents paramètres précédement traité vers les drivers (horloge [STEP] & Direction [DIR])  
    digitalWrite(DIR_FrontStepperMotor,not(DirClock));            // Ecriture des différents paramètres précédement traité vers les drivers (horloge [STEP] & Direction [DIR])  
  
  } 
}

void Task2code( void * parameter ){                               // Tache 2
  Serial.print("Task2 is running on core ");                      // Affichage d'un message dans la console
  Serial.println(xPortGetCoreID());                               // Affichage du coeur en plein execution dans la console
  const TickType_t xDelayTask1 = 500 / portTICK_PERIOD_MS ;       // Creation d'un délai de 500ms pour cadencer la boucle suivante
  
  for(;;){ 

    vTaskDelay( xDelayTask1 );                                    // Delai de 500ms
    
    // CAPTEUR ULTRASONIQUE
    int uS = sonar.ping();                                        // On récupère le temps de réflexion d'un obstacle après avoir lancé un signal ultrasonique          
    distance = uS / US_ROUNDTRIP_CM;                              // On calcule la distance par le biais de ce laps de temps récupéré

    // AFFICHAGE OLED
    sprintf(chardistance, "%5u  ", distance);                     // Formatage de cette distance en cm en char
    printToOLED(50, 52, chardistance);                            // Affichage dans l'OLED
    sprintf(charKmHeure, "%0.2f  ", KmHeure);                     // Formatage de la vitesse en km/h en char
    printToOLED(50, 5, charKmHeure);                              // Affichage dans l'OLED
    sprintf(LiSw1Char, "%2u", LiSw1Value);                        // Formatage de l'état du fin de course en char
    printToOLED(100, 5, LiSw1Char);                               // Affichage dans l'OLED
    sprintf(LiSw2Char, "%2u", LiSw2Value);                        // Formatage de l'état du fin de course en char
    printToOLED(100, 17, LiSw2Char);                              // Affichage dans l'OLED
    sprintf(LiSw3Char, "%2u", LiSw3Value);                        // Formatage de l'état du fin de course en char
    printToOLED(100, 40, LiSw3Char);                              // Affichage dans l'OLED
    sprintf(LiSw4Char, "%2u", LiSw4Value);                        // Formatage de l'état du fin de course en char
    printToOLED(100, 52, LiSw4Char);                              // Affichage dans l'OLED

    // COMMUNICATION/EMISSION
    client.publish("speed", charKmHeure);                         // On publie sur un topic la vitesse en km/h en chaîne de caractères
    client.publish("distance", chardistance);                     // On publie sur un topic la distance ultrasonique en cm en chaîne de caractères      
  }
}

void loop() {
  
}