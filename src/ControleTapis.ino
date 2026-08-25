
// --- Définition des broches ---
// Broche PWM connectée à la Gate du MOSFET
const int mosfetPin = 10; 

// --- Variables d'état ---
int puissanceChauffe = 0; // Puissance actuelle (0 à 255)

void chgPuissance(int mosfetPin, int pourcentage){
  int puisChauff = map(pourcentage, 0, 100, 0, 255);
  analogWrite(mosfetPin, puisChauff); // Met la nouvelle puissance en marche

  Serial.print(">> Puissance de chauffe réglée sur : ");
  Serial.print(pourcentage);
  Serial.println("%");
}



void setup() {
  // Initialisation de la communication série à 9600 bauds
  Serial.begin(9600);
  
  // Configuration de la broche du MOSFET en sortie
  pinMode(mosfetPin, OUTPUT);
  
  // Par sécurité, on s'assure que le chauffage est éteint au démarrage
  analogWrite(mosfetPin, 0); 
  
  Serial.println("--- Système de Chauffage Prêt ---");
  Serial.println("Entrez une valeur entre 0 (Éteint) et 100 (Max) pour régler la chauffe.");
}

void loop() {
  // Vérifie si des données ont été envoyées depuis l'ordinateur
  if (Serial.available() > 0) {
    
    // Lit la valeur entrée par l'utilisateur (se termine par 'Entrée')
    int pourcentage = Serial.parseInt();
    
    // Nettoie le buffer série des éventuels retours à la ligne restants
    Serial.readStringUntil('\n');

    // Sécurité : On s'assure que la valeur entrée est bien entre 0 et 100
    if (pourcentage >= 0 && pourcentage <= 100) {
      chgPuissance(mosfetPin, pourcentage);
    } 
    else {
      // Si l'utilisateur tape une valeur invalide (ex: 150)
      Serial.println("Erreur : La valeur doit être comprise entre 0 et 100.");
    }
  }
}
