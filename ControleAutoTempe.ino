#include <math.h>

// Pin des sorties et entrée de l'arduino
const int peltierPin = 12; // Pin de controle du module Peltier
const int plaquePin = 10; // Pin de controle de la plaque
const int pontDiv = A0; // Pin de lecture analogique du pont diviseur de tension
const int alimPin = 22; // Pin de controle l'alimentation du capteur et éviter sa chauffe

// Constante pour le calcul de la température (formule de Steinhart-Hart)
const float R1 = 10000.0;
const float A = 1.125e-3;
const float B = 2.347e-4;
const float C = 8.563e-8;

// Constante de temps pour le calcul de la température (diminution nécéssaire pour plus de précision)
const int tTemp = 1000; // Temps de prise température de la thermistance
unsigned long tRef = 0; // Initialisation du temps

// Variables d'etat et autres
bool modeAuto = false; // Choix du mode (Auto/Manuel)
float tempCible = 30; // Choix de la température cible
float marge = 0.25; // Choix de la marge pour éviter l'allumage et etindre constament


// Fonctions
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
void Peltier(int peltierPin, String command){
  if (command == "ON") {
    analogWrite(peltierPin, 0); // Il faut inverser par rapport à ce qui est écrit
    Serial.print(">> Peltier est allumée. |");
  }
  else if (command == "OFF") {
    analogWrite(peltierPin, 255);
    Serial.print(">> Peltier est éteint. |");
  }
  else if (command.length() > 0){
    Serial.print("Commande inconnue: '");
    Serial.print(command);
    Serial.print("'. Ecrivez 'PELTIER ON' ou 'PELTIER OFF'.");
  }
}

void Plaque(int plaquePin, int pourcentage){
  pourcentage = constrain(pourcentage, 0, 100); // Limitation du pourcentage pour le garder entre 0 et 100%

  int puisChauff = map(pourcentage, 0, 100, 0, 255);
  analogWrite(plaquePin, puisChauff); // Met la nouvelle puissance en marche

  Serial.print(">> Puissance de chauffe réglée sur : ");
  Serial.print(pourcentage);
  Serial.print("%. |");
}

void thermostat(float TC, float marge, int TCible, int peltierPin, int plaquePin){
  if (TC < (tempCible - marge)) {
    Plaque(plaquePin, 255); // On allume la plaque
    Peltier(peltierPin, "OFF"); // On éteint le peltier (logique inversée)
  }
  else if (TC > (tempCible + marge)) {
    Plaque(plaquePin, 0); // On éteint la plaque
    Peltier(peltierPin, "ON"); // On allume le peltier (logique inversée)
  }
  else {
    // Température dans la marge on éteint tout
    Plaque(plaquePin, 0);
    Peltier(peltierPin, "OFF");
  }
}



//Initialisation des composantes
void setup() {
  Serial.begin(9600);

  //Activation des sorties
  pinMode(alimPin, OUTPUT); // Sortie du diviseur de tension
  pinMode(peltierPin, OUTPUT); // Sortie pour le transformateur
  pinMode(plaquePin, OUTPUT); // Sortie de la plaque chauffante

  // Eteinte de ces sorties
  digitalWrite(alimPin, LOW);
  Peltier(peltierPin, "OFF");
  Plaque(plaquePin, 0);

  Serial.println("--- Systeme Pret ---");
  Serial.println("Taper 'AUTO' pour activer le mode automatique");
  Serial.println("Taper 'MANUEL' pour activer le mode manuel");
  Serial.println("Taper 'CIBLE 25' pour regler la température cible à 25º");
  Serial.println("Taper 'PELTIER ON' or 'PELTIER OFF' or 'TAPIS 0-100' pour activer le mode manuel et régler la chauffe et le refroidissement manuellement.");
  Serial.println("--------------------");
}

void loop() {
  unsigned long tAct = millis();

  if (tAct - tRef >= tTemp) {
    tRef = tAct;
    
    float R = valeurR(alimPin, pontDiv); // R prend la valeur de la résistance

    if (R > 0) {
      // Calcul de la température en ºC
      float logR = log(R);
      float invT = A + B * logR + C*logR*logR*logR;
      float TC  = (1 / invT) - 273.15;
    
      // Affichage de la température
      Serial.print("Temperature: ");
      Serial.print(TC);
      Serial.println(" °C.");

      if(modeAuto) {
        Serial.print("MODE AUTOMATIQUE | CIBLE : ");
        Serial.print(tempCible);
        Serial.print(" °C. |");

        thermostat(TC, marge, tempCible, peltierPin, plaquePin);
      }
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
    if (command == "AUTO") {
      modeAuto = true;
      Serial.println(">> MODE AUTOMATIQUE ACTIVE");
    }
    else if (command == "MANUEL") {
      modeAuto = false;
      Plaque(plaquePin, 0);
      Peltier(peltierPin, "OFF");
      Serial.println(">> MODE MANUEL ACTIVE");
    }
    else if (command.startsWith("CIBLE ")) {
      modeAuto = true;
      tempCible = command.substring(6).toInt();
      Serial.print(">> NOUVELLE CIBLE : ");
      Serial.print(tempCible);
      Serial.println(" °C (Mode Auto active).");
    }
    else if (command.startsWith("PELTIER ")) {
      modeAuto = false;
      String commande = command.substring(8);
      Peltier(peltierPin, commande);
    }
    else if (command.startsWith("TAPIS ")){
      modeAuto = false;
      int pourcentage = command.substring(6).toInt();
      Plaque(plaquePin, pourcentage);
    }
    else if (command.length() > 0){
      Serial.print("Commande inconnue: '");
      Serial.print(command);
      Serial.println("'. Tapez 'PELTIER ON' or 'PELTIER OFF' or 'TAPIS pourcentage'.");
    }
  }
}