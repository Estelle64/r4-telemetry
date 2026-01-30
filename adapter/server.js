const Broker = require("./broker");
const { MongoClient } = require("mongodb");
const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const path = require("path");
const crypto = require("crypto");
const stringify = require("json-stable-stringify");

// --- Configuration ---
// Conflict resolution: Keeping the IP that works for the current setup, or ENV var.
const BROKER_URL = process.env.MQTT_BROKER_URL || "mqtt://10.191.64.101:1883";
const TOPICS = ["cesi/cafet", "cesi/fablab"];
const MONGO_URL = process.env.MONGO_URL || "mongodb://localhost:27017";
const DB_NAME = "telemetryDb";
const COLLECTION_NAME = "blockchain_readings";
const PORT = process.env.PORT || 3000;

// Genesis Hash (SHA256 of "Genesis Block")
const GENESIS_HASH =
  "0000000000000000000000000000000000000000000000000000000000000000";

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

/**
 * Calculates the SHA256 hash of a block.
 * Uses json-stable-stringify to ensure deterministic output for the data object.
 */
function calculateHash(index, previousHash, timestamp, location, data) {
  const dataString = stringify(data);
  const content = index + previousHash + timestamp + location + dataString;
  return crypto.createHash("sha256").update(content).digest("hex");
}

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

    // Ensure index for sorting
    await collection.createIndex({ index: -1 });

    console.log("Connexion au broker MQTT...");
    const broker = new Broker(BROKER_URL);

    TOPICS.forEach((subscribedTopic) => {
      broker.subscribe(subscribedTopic, async (topic, message) => {
        let parsedMqttData;
        try {
          parsedMqttData = JSON.parse(message.toString());
        } catch (e) {
          console.error("Invalid JSON received:", message.toString());
          return;
        }

        console.log(`Parsed MQTT data for topic ${topic}:`, parsedMqttData);

        try {
          const location = topic.includes("cafet")
            ? "cafet"
            : topic.includes("fablab")
              ? "fablab"
              : "unknown";

          if (location !== "unknown" && latestLocationData[location]) {
            // Update local state (cache)

            if (parsedMqttData.temperature !== undefined) {
              latestLocationData[location].remoteTemp =
                parsedMqttData.temperature;
            }
            if (parsedMqttData.humidity !== undefined) {
              latestLocationData[location].remoteHum = parsedMqttData.humidity;
            }
            // Fallback for older packet format
            if (parsedMqttData.localTemp !== undefined)
              latestLocationData[location].localTemp = parsedMqttData.localTemp;
            if (parsedMqttData.localHum !== undefined)
              latestLocationData[location].localHum = parsedMqttData.localHum;

            latestLocationData[location].lastUpdate = new Date().getTime();
            if (parsedMqttData.loraStatus !== undefined)
              latestLocationData[location].loraStatus =
                parsedMqttData.loraStatus === "true" ||
                parsedMqttData.loraStatus === true;
            if (parsedMqttData.dhtStatus !== undefined)
              latestLocationData[location].dhtStatus =
                parsedMqttData.dhtStatus === "true" ||
                parsedMqttData.dhtStatus === true;
            if (parsedMqttData.timeSynced !== undefined)
              latestLocationData[location].timeSynced =
                parsedMqttData.timeSynced === "true" ||
                parsedMqttData.timeSynced === true;
          }

          // --- BLOCKCHAIN INSERTION LOGIC ---

          // 1. Get the last block to find previousHash
          const lastBlock = await collection.findOne(
            {},
            { sort: { index: -1 } },
          );

          const newIndex = lastBlock ? lastBlock.index + 1 : 0;
          const previousHash = lastBlock ? lastBlock.hash : GENESIS_HASH;
          const timestamp = new Date().toISOString();
          const dataToStore = { ...latestLocationData[location] };

          // 2. Calculate Hash
          const newHash = calculateHash(
            newIndex,
            previousHash,
            timestamp,
            location,
            dataToStore,
          );

          // 3. Create Block
          const newBlock = {
            index: newIndex,
            timestamp: timestamp,
            location: location,
            data: dataToStore,
            previousHash: previousHash,
            hash: newHash,
          };

          console.log(
            `[Blockchain] New Block mined: Index ${newIndex} | Hash: ${newHash.substring(0, 15)}...`,
          );

          await collection.insertOne(newBlock);
          console.log(
            `Données enregistrées dans MongoDB (Blockchain) pour ${location}.`,
          );

          // Broadcast to all WebSocket clients
          wss.clients.forEach((client) => {
            if (client.readyState === WebSocket.OPEN) {
              client.send(JSON.stringify(newBlock));
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
