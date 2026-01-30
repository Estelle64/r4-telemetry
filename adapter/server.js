const Broker = require("./broker");
const { MongoClient } = require("mongodb");
const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const path = require("path");

// --- Configuration ---
const BROKER_URL = process.env.MQTT_BROKER_URL || "mqtt://172.20.10.12:1883";
const TOPICS = ["cesi/cafet", "cesi/fablab"];
const MONGO_URL = process.env.MONGO_URL || "mongodb://localhost:27017";
const DB_NAME = "telemetryDb";
const COLLECTION_NAME = "readings";
const PORT = process.env.PORT || 3000;

// Global state to store the latest sensor data for each location
const latestLocationData = {
  cafet: {
    remoteTemp: 0.0,
    remoteHum: 0.0,
    loraStatus: true,
    dhtStatus: true,
    timeSynced: false,
    lastUpdate: 0,
    localTemp: 0.0,
    localHum: 0.0,
  },
  fablab: {
    remoteTemp: 0.0,
    remoteHum: 0.0,
    loraStatus: true,
    dhtStatus: true,
    timeSynced: false,
    lastUpdate: 0,
    localTemp: 0.0,
    localHum: 0.0,
  },
};

async function main() {
  // --- Express & WebSocket Setup ---
  const app = express();
  const server = http.createServer(app);
  const wss = new WebSocket.Server({ server });

  app.use(express.static(path.join(__dirname, "public")));

  wss.on("connection", (ws) => {
    console.log("Client WebSocket connecté");
    ws.on("close", () => {
      console.log("Client WebSocket déconnecté");
    });
  });

  console.log("Connexion à la base de données MongoDB...");
  const mongoClient = new MongoClient(MONGO_URL);

  try {
    await mongoClient.connect();
    console.log("Connecté à MongoDB.");

    const db = mongoClient.db(DB_NAME);
    const collection = db.collection(COLLECTION_NAME);

    console.log("Connexion au broker MQTT...");
    const broker = new Broker(BROKER_URL);

    TOPICS.forEach((subscribedTopic) => {
      broker.subscribe(subscribedTopic, async (topic, message) => {
        console.log(`Message reçu sur le topic ${topic}:`, message.toString());
        const parsedMqttData = JSON.parse(message.toString());
        console.log(`Parsed MQTT data for topic ${topic}:`, parsedMqttData);

        try {
          const location = topic.includes("cafet")
            ? "cafet"
            : topic.includes("fablab")
              ? "fablab"
              : "unknown";

          if (location !== "unknown" && latestLocationData[location]) {
            console.log(`Latest data for ${location} BEFORE update:`, {
              ...latestLocationData[location],
            });
            if (parsedMqttData.data_type === "temp") {
              latestLocationData[location].remoteTemp = parsedMqttData.value;
              latestLocationData[location].lastUpdate = new Date().getTime();
            } else if (parsedMqttData.data_type === "hum") {
              latestLocationData[location].remoteHum = parsedMqttData.value;
              latestLocationData[location].lastUpdate = new Date().getTime();
            }
            // For now, we assume localTemp and localHum are 0.0 and will not be updated via MQTT
            // loraStatus, dhtStatus, timeSynced are assumed to be static or updated by other means
            console.log(`Latest data for ${location} AFTER update:`, {
              ...latestLocationData[location],
            });
          }

          const newEntry = {
            timestamp: new Date(),
            location: location, // Add location to the entry
            data: { ...latestLocationData[location] }, // Use the consolidated data
          };
          console.log(
            `Final newEntry object to be sent via WebSocket for topic ${topic}:`,
            newEntry,
          );

          await collection.insertOne(newEntry);
          console.log(`Données enregistrées dans MongoDB pour ${location}.`);

          // Broadcast to all WebSocket clients
          wss.clients.forEach((client) => {
            if (client.readyState === WebSocket.OPEN) {
              client.send(JSON.stringify(newEntry));
            }
          });
        } catch (error) {
          console.error(
            "Erreur lors du traitement du message ou de l'insertion en base de données:",
            error,
          );
        }
      });
    });

    console.log(
      `Adaptateur démarré. Écoute des topics: ${TOPICS.join(", ")}...`,
    );
  } catch (err) {
    console.error(
      "Erreur de connexion à MongoDB. Assurez-vous que le serveur est lancé.",
      err,
    );
    process.exit(1);
  }

  server.listen(PORT, () => {
    console.log(`Serveur web démarré sur http://localhost:${PORT}`);
  });

  // Gérer la déconnexion propre
  process.on("SIGINT", async () => {
    console.log("Déconnexion de MongoDB...");
    await mongoClient.close();
    console.log("Adaptateur arrêté.");
    process.exit(0);
  });
}

main().catch(console.error);
