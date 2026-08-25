#include <math.h>

// Constante de temps
const float R1 = 10000.0; // Résistance connue en Ohms (par exemple, 10k pour 10000)
const int echant = 1000; // Temps d'échantillonage en ms
// Coefficient de la formule de Steihart-Hart
const float A = 1.125e-3;
const float B = 2.347e-4;
const float C = 8.563e-8;


const int pontDiv = A0; // Broche de lecture analogique du pont diviseur de tension
const int pinAlimDiv = 22; //Pin de sortie pour controller l'alimentation du capteur et éviter sa chauffe

unsigned long tRef = 0; // Initialisation du temps


float valeurR(int pinAlim,int pinDiv){
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


void setup() {
  Serial.begin(9600);
  pinMode(pinAlimDiv, OUTPUT); // Output de tension
}

void loop() {
  unsigned long tAct = millis();

  if (tAct - tRef >= echant) {
    tRef = tAct;
    
    float R = valeurR(pinAlimDiv, pontDiv); // Calcul de R

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
}
