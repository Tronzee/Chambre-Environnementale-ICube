#include <math.h>

// Constantes tenant compte des sorties et entrée de l'arduino
const int pwmPin = 2; // pin ou est connecté l'alim du module Peltier
const int pontDiv = A0; // Broche de lecture analogique du pont diviseur de tension
const int pinAlimDiv = 22; //Pin de sortie pour controller l'alimentation du capteur et éviter sa chauffe

// Constante pour le calcul de la température
const float R1 = 10000.0; // Résistance connue en Ohms 
const float A = 1.125e-3; // Formule de Steinhart-Hart
const float B = 2.347e-4;
const float C = 8.563e-8;

// Constante de temps pour le calcul de la température
const int tTemp = 1000; // Temps de prise température de la thermistance

// Initialisation des variables
int isPeltierOn = 0; // Initialisation du Peltier éteint
unsigned long tRef = 0; // Initialisation du temps


// Fonction utilisé
// Fonction calculant la valeur de la résistance
float valeurR(int pinAlim, int pinDiv){
  digitalWrite(pinAlim, HIGH); // Allume le diviseur de tension
  delay(10); // Donne un peu de temps avant de faire la mesure

  int valeurBrute = analogRead(pinDiv); // Lecture de la tension du pont div de tension. Renvoie une valeur entre 0 et 1023

  digitalWrite(pinAlim, LOW); // Éteint le diviseur de tension

  if (valeurBrute > 0){
    float tension = (valeurBrute * 5.0) / 1023.0; // Tension réelle mesurée
    float rInconnue = R1 / ((5.0 / tension) - 1.0); // Calcul de la résistance inconnue

    return rInconnue;
  }
  else{
    return -1;
  }
}

// Fonction qui change l'état du Peltier
int chgEtat(String command, int isPeltierOn){
  if (command == "ON") {
    analogWrite(pwmPin, 0); // Il faut inverser par rapport à ce qui est écrit
    Serial.println(">> Peltier is now ON");
    return 1;
  }
  else if (command == "OFF") {
    analogWrite(pwmPin, 255);
    Serial.println(">> Peltier is now OFF");
    return 0;
  }
  else if (command.length() > 0){
    Serial.print("Unknown command: '");
    Serial.print(command);
    Serial.println("'. Please type 'ON' or 'OFF'.");
    return isPeltierOn;
  }
}



//Initialisation des composantes
void setup() {
  Serial.begin(9600);
  pinMode(pinAlimDiv, OUTPUT); // Sortie du diviseur de tension
  pinMode(pwmPin, OUTPUT); // Sortie pour le transformateur

  // Turn driver fully OFF on startup (Inverted logic: HIGH = OFF)
  analogWrite(pwmPin, 255);

  Serial.println("--- System Ready ---");
  Serial.println("Type 'ON' to start the Peltier.");
  Serial.println("Type 'OFF' to stop the Peltier.");
}

void loop() {
  unsigned long tAct = millis();

  if (tAct - tRef >= tTemp) {
    tRef = tAct;
    
    float R = valeurR(pinAlimDiv, pontDiv); // R prend la valeur de la résistance

    if (R > 0) {
      float logR = log(R);
      float invT = A + B * logR + C*logR*logR*logR; // Calcule de l'inverse de la température
      float T = 1 / invT; // Température en Kelvin
      float TC = T - 273.15;
    
      // Affichage de la température
      Serial.print("Temperature: ");
      Serial.print(TC);
      Serial.println(" °C");
    }
    else {
      Serial.print("Problème avec le calcul de résistance.");
    }
  }

  if (Serial.available() > 0){
    String command = Serial.readStringUntil('\n');
    
    command.trim(); // enleve les espaces
    command.toUpperCase(); // met tout le texte en majuscule

    // Process the command
    isPeltierOn = chgEtat(command, isPeltierOn);
  }

}
