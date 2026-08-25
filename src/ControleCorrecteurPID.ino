#include <math.h>

// --- PIN ---
const int peltierPin = 12;      // Pin de commande du module Peltier (Recom)
const int plaquePin = 10;       // Pin de controle du MOSFET (Tapis)
const int pontDiv = A0;         // Pin de lecture analogique du pont diviseur de tension
const int alimPin = 22;         // Pin d'alimentation du capteur (pour éviter l'auto-échauffement)

// --- Constantes Steinhart-Hart (Thermistance) ---
const float R1 = 10000.0; 
const float A = 1.125e-3; 
const float B = 2.347e-4;
const float C = 8.563e-8;

// --- Paramètres de temps ---
const int tTemp = 1000; // Fréquence de calcul PID et température (1000ms = 1s)
unsigned long tRef = 0; 

// --- Variables d'état ---
bool modeAuto = false;   
float tempCible = 25.0;  

// ==============================================================================
// VARIABLES DU PID
// ==============================================================================
float Kp = 50.0;  // Proportionnel : Puissance brute (plus on est loin, plus on pousse fort)
float Ki = 2.0;   // Intégral : Corrige les petites erreurs qui s'accumulent dans le temps
float Kd = 10.0;  // Dérivé : Agit comme un "frein" si la température change trop vite

float erreurPrecedente = 0.0; // Pour calculer le coefficient Dérivé
float sommeErreurs = 0.0;     // Pour l'accumulation du coefficient Intégral
// ==============================================================================


// --- Fonction de lecture de la résistance ---
float valeurR(int pinAlim, int pinDiv){
  digitalWrite(pinAlim, HIGH); 
  delay(10); 
  int valeurBrute = analogRead(pinDiv); 
  digitalWrite(pinAlim, LOW); 

  if (valeurBrute > 0){
    float tension = (valeurBrute * 5.0) / 1023.0; 
    return R1 / ((5.0 / tension) - 1.0);          // Retourne la valeur de résistance de la thermistance
  }
  return -1;
}

// ==============================================================================
// INITIALISATION
// ==============================================================================
void setup() {
  Serial.begin(9600);
  
  //Activation des sorties
  pinMode(alimPin, OUTPUT);    // Sortie du diviseur de tension
  pinMode(peltierPin, OUTPUT); // Sortie pour le transformateur
  pinMode(plaquePin, OUTPUT);  // Sortie de la plaque chauffante

  // Eteinte de ces sorties
  digitalWrite(alimPin, LOW);
  digitalWrite(peltierPin, 255);
  digitalWrite(plaquePin, 0);

  Serial.println("--- Systeme PID Pret ---");
  Serial.println("- Taper 'AUTO' pour lancer le PID.");
  Serial.println("- Taper 'OFF' pour couper le PID.");
  Serial.println("- Taper 'CIBLE 25' pour changer la consigne.");
  Serial.println("- Taper 'KP 60', 'KI 5' ou 'KD 15' pour regler le PID.");
  Serial.println("------------------------");
}

// ==============================================================================
// BOUCLE PRINCIPALE
// ==============================================================================
void loop() {
  unsigned long tAct = millis();

  // 1. TACHE : CALCUL PID & TEMPERATURE (Toutes les secondes)
  if (tAct - tRef >= tTemp) {
    tRef = tAct;
    
    float R = valeurR(alimPin, pontDiv); 

    if (R > 0) {
      // -- Calcul Température --
      float logR = log(R);
      float invT = A + B * logR + C*logR*logR*logR; 
      float TC = (1 / invT) - 273.15;
    
      Serial.print("Temp: ");
      Serial.print(TC);
      Serial.print(" °C.");
      
      // -- ALGORITHME PID --
      if (modeAuto) {
        Serial.print("CORRECTEUR ON | CIBLE : ");
        Serial.print(tempCible);
        Serial.print(" °C. |");

        // 1. L'erreur actuelle (Cible - Température actuelle)
        float erreur = tempCible - TC;
        
        // 2. Terme Intégral (Accumulation des erreurs)
        sommeErreurs += erreur;
        
        // Sécurité Anti-Windup : On empêche l'intégrale de grossir à l'infini
        // On limite la somme pour qu'elle ne dépasse pas l'impact maximum (255)
        float limiteInt = 255.0 / (Ki > 0.001 ? Ki : 1.0);
        sommeErreurs = constrain(sommeErreurs, -limiteInt, limiteInt);
        
        // 3. Terme Dérivé (Vitesse de changement de l'erreur)
        float variationErreur = erreur - erreurPrecedente;
        
        // 4. CALCUL GLOBAL PID
        float commandePID = (Kp * erreur) + (Ki * sommeErreurs) + (Kd * variationErreur);
        
        // On sauvegarde l'erreur pour le prochain cycle
        erreurPrecedente = erreur;
        
        // On limite la commande entre -255 (Peltier max) et +255 (Tapis max)
        commandePID = constrain(commandePID, -255, 255);
        
        // 5. APPLICATION AUX ACTIONNEURS
        if (commandePID > 0) {
          // Action positive = On doit CHAUFFER
          int pwmTapis = (int)commandePID;
          analogWrite(plaquePin, pwmTapis);
          analogWrite(peltierPin, 255); // Peltier eteint
          
          Serial.print(" | Action: CHAUFFE (");
          Serial.print(pwmTapis);
          Serial.println("/255)  ");
          
        } else if (commandePID < 0) {
          // Action négative = On doit REFROIDIR
          int pwmPeltier = (int)abs(commandePID);
          analogWrite(plaquePin, 0); // Tapis OFF
          // Logique inversée pour le Recom : 255 - PWM désiré
          analogWrite(peltierPin, 255 - pwmPeltier); 
          
          Serial.print(" | Action: FROID (");
          Serial.print(pwmPeltier);
          Serial.println("/255)");
          
        } else {
          // Température absolument parfaite on éteint tout!
          analogWrite(plaquePin, 0);
          analogWrite(peltierPin, 255);
          Serial.println(" | Action: REPOS");
        }
      } else {
        Serial.println(); // Retour à la ligne en mode manuel
      }
    }
  }

  // 2. TACHE : LECTURE DES COMMANDES SERIE
  if (Serial.available() > 0){
    String command = Serial.readStringUntil('\n');
    command.trim(); 
    command.toUpperCase(); 

    if (command == "AUTO") {
      modeAuto = true;
      // Remise à zéro du PID au démarrage pour éviter les à-coups
      sommeErreurs = 0;
      float R = valeurR(alimPin, pontDiv);
      erreurPrecedente = tempCible - ( (1 / (A + B * log(R) + C*pow(log(R), 3))) - 273.15 );
      Serial.println(">> MODE AUTO PID ACTIVE");
    }
    else if (command == "OFF") {
      modeAuto = false;
      analogWrite(plaquePin, 0);
      analogWrite(peltierPin, 255);
      Serial.println(">> PID DESACTIVE (Composants coupes)");
    }
    else if (command.startsWith("CIBLE ")) {
      tempCible = command.substring(6).toFloat();
      sommeErreurs = 0; // Reset de l'intégrale pour le changement de cap
      modeAuto = true;
      Serial.print(">> NOUVELLE CIBLE : ");
      Serial.println(tempCible);
    }
    // --- COMMANDES DE REGLAGE DU PID ---
    else if (command.startsWith("KP ")) {
      Kp = command.substring(3).toFloat();
      Serial.print(">> Kp modifie : ");
      Serial.println(Kp);
    }
    else if (command.startsWith("KI ")) {
      Ki = command.substring(3).toFloat();
      sommeErreurs = 0; // Reset
      Serial.print(">> Ki modifie : ");
      Serial.println(Ki);
    }
    else if (command.startsWith("KD ")) {
      Kd = command.substring(3).toFloat();
      Serial.print(">> Kd modifie : ");Serial.println(Kd);
    }
  }
}
