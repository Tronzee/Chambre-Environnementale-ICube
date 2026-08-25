import serial
import matplotlib.pyplot as plt
import time
from datetime import datetime

# --- CONFIGURATION DU PORT ---
port = 'COM7'
baud = 9600
duree_acquisition = 1200  # 600 secondes = 10 minutes
fenetre_affichage = 60   # Affiche les 60 dernières secondes sur le graphe

print("=========================================")
print("   CONFIGURATION DE L'ESSAI (30 MIN)     ")
print("=========================================")

# 1. SAISIE DES PARAMÈTRES PAR L'UTILISATEUR
try:
    val_kp = float(input("Entrez la valeur de Kp : "))
    val_ki = float(input("Entrez la valeur de Ki : "))
    val_kd = float(input("Entrez la valeur de Kd : "))
    val_cible = float(input("Entrez la température cible (°C) : "))
except ValueError:
    print("Erreur : Vous devez entrer des nombres valides.")
    exit()

# 2. CONNEXION À L'ARDUINO
try:
    ser = serial.Serial(port, baud, timeout=0.1)
    time.sleep(2) # Laisse le temps à l'Arduino de redémarrer suite à la connexion
except Exception as e:
    print(f"Erreur de connexion au port {port} : {e}")
    exit()

# 3. ENVOI DES PARAMÈTRES À L'ARDUINO
print("\nTransmission des paramètres à l'Arduino...")
ser.write((f"KP {val_kp}\n").encode('utf-8'))
time.sleep(0.1)
ser.write((f"KI {val_ki}\n").encode('utf-8'))
time.sleep(0.1)
ser.write((f"KD {val_kd}\n").encode('utf-8'))
time.sleep(0.1)
ser.write((f"CIBLE {val_cible}\n").encode('utf-8'))
time.sleep(0.1)

# Création du fichier de sauvegarde
nom_fichier = datetime.now().strftime("donnees_10min_%Y%m%d_%H%M%S.txt")
fichier = open(nom_fichier, "w", encoding="utf-8")
fichier.write(f"# --- TEST 10 MIN | Kp={val_kp} | Ki={val_ki} | Kd={val_kd} | Cible={val_cible}\n")
fichier.write("Temps(s),Temperature_C,Cible_C,Etat,Kp,Ki,Kd\n")

# Lancement du PID
ser.write(("AUTO\n").encode('utf-8'))
print(f">> PID ACTIVÉ. Enregistrement en cours dans : {nom_fichier}")

# 4. INITIALISATION DU GRAPHIQUE
x_temps, y_temp, y_cible = [], [], []
plt.ion()
fig, ax = plt.subplots()

compteur = 0
heure_debut = time.time()

# 5. BOUCLE D'ACQUISITION (10 MINUTES)
try:
    while True:
        temps_ecoule = time.time() - heure_debut
        temps_restant = duree_acquisition - temps_ecoule
        
        # Vérification de la fin du temps imparti
        if temps_restant <= 0:
            print("\n=========================================")
            print("   TEMPS ÉCOULÉ (10 MIN) - FIN DU TEST   ")
            print("=========================================")
            break # On sort de la boucle principale

        # Lecture des données de l'Arduino
        ligne = ser.readline().decode('utf-8', errors='ignore').strip()
        
        if ligne and "," in ligne:
            parts = ligne.split(',')
            if len(parts) >= 6:
                try:
                    tc = float(parts[0])
                    cible = float(parts[1])
                    etat = parts[2].strip()
                    current_kp = float(parts[3])
                    current_ki = float(parts[4])
                    current_kd = float(parts[5])
                    
                    # Sauvegarde dans le fichier
                    ligne_txt = f"{compteur},{tc},{cible},{etat},{current_kp},{current_ki},{current_kd}\n"
                    fichier.write(ligne_txt)
                    fichier.flush() 
                    
                    # Mise à jour des listes
                    x_temps.append(compteur)
                    y_temp.append(tc)
                    y_cible.append(cible)
                    compteur += 1
                    
                    # Dessin du graphique (fenêtre glissante de 60s)
                    ax.clear()
                    ax.plot(x_temps[-fenetre_affichage:], y_temp[-fenetre_affichage:], label="Température (°C)", color="blue")
                    ax.plot(x_temps[-fenetre_affichage:], y_cible[-fenetre_affichage:], label="Cible (°C)", color="red", linestyle="--")
                    
                    titre = f"Reste: {int(temps_restant)}s | État: {etat} | PID({current_kp}, {current_ki}, {current_kd})"
                    ax.set_title(titre, fontsize=12, fontweight="bold")
                    ax.legend(loc="upper left") 
                    
                except ValueError:
                    pass
        
        # Rafraîchissement de la fenêtre
        plt.pause(0.01)
                
except KeyboardInterrupt:
    print("\n[INTERRUPTION MANUELLE] Arrêt prématuré de l'expérience.")
except Exception as e:
    print(f"\n[ERREUR] {e}")

# 6. PROCÉDURE DE FIN ET SÉCURITÉ
fichier.close()
ser.write(('OFF\n').encode('utf-8')) # On coupe la plaque et le Peltier
print(">> PID COUPÉ PAR SÉCURITÉ.")
print(f">> Données sauvegardées dans : {nom_fichier}")

# On désactive le mode interactif pour garder le graphique final affiché à l'écran
plt.ioff()
ax.set_title("TEST TERMINÉ", fontsize=14, fontweight="bold", color="green")
plt.show() # La fenêtre reste ouverte jusqu'à ce que vous la fermiez manuellement
