const mqtt = require("mqtt");

const BROKER = "mqtt://172.20.10.7:1883";

const client = mqtt.connect(BROKER);

client.on("connect", () => {
  console.log("Connecté au broker");

  client.subscribe("test/topic", () => {
    console.log("Abonné à test/topic");
  });
});

client.on("message", (topic, message) => {
  console.log("Reçu:", topic, message.toString());
});
