const mosquitto = require('mqtt');

class Broker {
  constructor(brokerUrl, options = {}) {
    this.client = mosquitto.connect(brokerUrl, options);
    this.subscriptions = new Map();

    this.client.on('message', (topic, message) => {
        if (this.subscriptions.has(topic)) {
            this.subscriptions.get(topic).forEach(callback => callback(topic, message));
        }
    });
  }
   subscribe(topic, callback) {
    if (!this.subscriptions.has(topic)) {
        this.subscriptions.set(topic, []);
        this.client.subscribe(topic);
    }
    this.subscriptions.get(topic).push(callback);
  }
    publish(topic, message) {
    this.client.publish(topic, message);
    }
}

module.exports = Broker;