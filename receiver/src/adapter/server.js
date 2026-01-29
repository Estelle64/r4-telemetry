const Broker = require("./broker");
const { MongoClient } = require("mongodb");

// --- Configuration ---
const BROKER_URL = process.env.MQTT_BROKER_URL || "mqtt://localhost:1883";
const TOPIC = "home/cafet/temp";
const MONGO_URL = process.env.MONGO_URL || "mongodb://localhost:27017";
const DB_NAME = "telemetryDb";
const COLLECTION_NAME = "readings";

async function main() {
  console.log("Connexion à la base de données MongoDB...");
  const mongoClient = new MongoClient(MONGO_URL);

  try {
    await mongoClient.connect();
    console.log("Connecté à MongoDB.");

    const db = mongoClient.db(DB_NAME);
    const collection = db.collection(COLLECTION_NAME);

    console.log("Connexion au broker MQTT...");
    const broker = new Broker(BROKER_URL);

    broker.subscribe(TOPIC, async (message) => {
      console.log(`Message reçu sur le topic ${TOPIC}:`, message.toString());

      try {
        const newEntry = {
          timestamp: new Date(),
          data: JSON.parse(message.toString()),
        };

        await collection.insertOne(newEntry);
        console.log("Données enregistrées dans MongoDB.");
      } catch (error) {
        console.error(
          "Erreur lors du traitement du message ou de l'insertion en base de données:",
          error,
        );
      }
    });

    console.log(`Adaptateur démarré. Écoute du topic "${TOPIC}"...`);
  } catch (err) {
    console.error(
      "Erreur de connexion à MongoDB. Assurez-vous que le serveur est lancé.",
      err,
    );
    process.exit(1);
  }

  // Gérer la déconnexion propre
  process.on("SIGINT", async () => {
    console.log("Déconnexion de MongoDB...");
    await mongoClient.close();
    console.log("Adaptateur arrêté.");
    process.exit(0);
  });
}

main().catch(console.error);
