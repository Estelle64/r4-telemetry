# Guide d'Installation - R4-Telemetry

Ce guide détaille les étapes pour déployer le système complet : les nœuds physiques (Arduino) et le backend (Docker).

## Prérequis

*   **Matériel** :
    *   2x Arduino R4 WiFi (ou compatible).
    *   2x Modules LoRa (SX1276/RFM95).
    *   1x Capteur DHT22 (ou DHT11).
*   **Logiciel** :
    *   [Arduino IDE](https://www.arduino.cc/en/software) (pour flasher les cartes).
    *   [Docker Desktop](https://www.docker.com/products/docker-desktop/) (pour le serveur).

---

## 1. Flasher l'Émetteur (Sender)

L'émetteur est le nœud distant (ex: Fablab) qui capte les données et les envoie via LoRa.

1.  Ouvrez le dossier `sender/` dans **Arduino IDE**.
2.  Ouvrez le fichier `sender.ino`.
3.  Vérifiez la configuration dans `src/config.h` (pins LoRa, type de capteur).
4.  Connectez votre Arduino R4 "Sender".
5.  Sélectionnez la bonne carte et le port COM.
6.  Cliquez sur **Téléverser (Upload)**.

---

## 2. Flasher le Récepteur (Gateway)

Le récepteur (ex: Cafétéria) reçoit les données LoRa, lit ses propres capteurs, et transmet tout au serveur via WiFi/MQTT.

1.  Ouvrez le dossier `receiver/` dans **Arduino IDE**.
2.  Ouvrez le fichier `receiver.ino`.
3.  **Configuration WiFi** : Ouvrez `src/config.h` et modifiez ces lignes avec vos identifiants :
    ```cpp
    #define WIFI_SSID "Votre_SSID"
    #define WIFI_PASS "Votre_Mot_De_Passe"
    #define MQTT_SERVER "IP_DE_VOTRE_PC" // Ex: 192.168.1.15
    ```
    > **⚠️ Important :** L'adresse IP doit être celle de votre ordinateur qui fera tourner Docker (pas `localhost`). Utilisez `ipconfig` (Windows) ou `ifconfig` (Mac/Linux) pour la trouver.
4.  Connectez votre Arduino R4 "Receiver".
5.  Cliquez sur **Téléverser (Upload)**.

---

## 3. Déployer le Serveur (Backend & Dashboard)

Le serveur gère la base de données, le broker MQTT et l'interface Web. Tout est conteneurisé avec Docker.

### ⚠️ Note pour les utilisateurs Windows
Si vous avez déjà installé Mosquitto en tant que service Windows, il faut l'arrêter car il entrera en conflit avec le Mosquitto de Docker.
*   Ouvrez "Services" (`Win + R` -> `services.msc`).
*   Cherchez "Mosquitto Broker".
*   Faites **Clic droit > Arrêter**.

### Installation et Démarrage

1.  Ouvrez un terminal à la racine du projet (`r4-telemetry/`).
2.  Lancez la commande suivante pour construire et démarrer tous les services :
    ```bash
    docker compose up --build -d
    ```
    *(Cette étape peut prendre quelques minutes la première fois car Docker télécharge les images et installe les dépendances npm automatiquement).*

3.  Vérifiez que tout tourne :
    ```bash
    docker compose ps
    ```

### Accéder au Dashboard

Une fois les conteneurs lancés, ouvrez votre navigateur :

*   **Dashboard Web** : [http://localhost](http://localhost)
*   **API (JSON)** : [http://localhost:3000/api/data](http://localhost:3000/api/data)

---

## Dépannage Rapide

*   **L'Arduino ne se connecte pas au MQTT :**
    *   Vérifiez que le **Pare-feu Windows** autorise le trafic entrant sur le port `1883`.
    *   Vérifiez que l'IP dans `receiver/src/config.h` est correcte et accessible depuis le réseau WiFi.
*   **Le Dashboard reste sur "Waiting for data..." :**
    *   Assurez-vous que l'Arduino Gateway est allumé et connecté au WiFi.
    *   Vérifiez les logs de l'adaptateur : `docker logs telemetry_adapter`.
