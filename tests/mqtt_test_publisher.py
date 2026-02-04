import paho.mqtt.client as mqtt
import json
import time
import random
import sys

# Configuration
BROKER = "10.191.64.108"
PORT = 1883
TOPICS = ["cesi/cafet", "cesi/fablab"]

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print(f"Failed to connect, return code {rc}")

def on_publish(client, userdata, mid):
    # print(f"Message {mid} published.")
    pass

def generate_payload():
    """Generates a random sensor data payload matching the adapter's expected format."""
    return {
        "temperature": round(random.uniform(10.0, 30.0), 2),  # Remote Temp
        "humidity": round(random.uniform(40.0, 90.0), 2),     # Remote Hum
        "localTemp": round(random.uniform(15.0, 25.0), 2),    # Local Temp
        "localHum": round(random.uniform(30.0, 60.0), 2),     # Local Hum
        "loraStatus": True,
        "dhtStatus": True,
        "timeSynced": random.choice([True, False])
    }

def main():
    # Fix for paho-mqtt 2.0+
    try:
        from paho.mqtt.enums import CallbackAPIVersion
        client = mqtt.Client(CallbackAPIVersion.VERSION1, "PythonTestPublisher")
    except ImportError:
        # Fallback for older paho-mqtt versions
        client = mqtt.Client("PythonTestPublisher")

    client.on_connect = on_connect
    client.on_publish = on_publish

    print(f"Connecting to {BROKER}:{PORT}...")
    try:
        client.connect(BROKER, PORT)
    except Exception as e:
        print(f"Error connecting to broker: {e}")
        print("Make sure the Mosquitto container is running and port 1883 is exposed.")
        sys.exit(1)

    client.loop_start()

    try:
        while True:
            for topic in TOPICS:
                payload = generate_payload()
                json_payload = json.dumps(payload)
                
                info = client.publish(topic, json_payload)
                info.wait_for_publish()
                
                print(f"Published to {topic}: {json_payload}")
            
            time.sleep(5)

    except KeyboardInterrupt:
        print("\nStopping publisher...")
    finally:
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
