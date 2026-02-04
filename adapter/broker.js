const mosquitto = require('mqtt');

class Broker {
  constructor(brokerUrl, options = {}) {
    this.client = mosquitto.connect(brokerUrl, options);
    this.subscriptions = new Map();

    this.client.on('message', (topic, message) => {
        console.log(`[Broker] Message reçu sur ${topic}: ${message.toString()}`);
        if (this.subscriptions.has(topic)) {
            this.subscriptions.get(topic).forEach(callback => callback(topic, message));
        }
    });
  }
   subscribe(topic, callback) {
    console.log(`[Broker] Subscribing to ${topic}`);
    if (!this.subscriptions.has(topic)) {
        this.subscriptions.set(topic, []);
        this.client.subscribe(topic, (err) => {
            if (err) console.error(`[Broker] Subscribe error for ${topic}:`, err);
            else console.log(`[Broker] Subscribed to ${topic} successfully`);
        });
    }
    this.subscriptions.get(topic).push(callback);
  }
    publish(topic, message) {
    this.client.publish(topic, message);
    }
}

module.exports = Broker;