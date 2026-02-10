# R4-Telemetry

**Bridging the gap between network-covered areas and white zones with LoRa and Arduino R4 WiFi.**

<p align="center">
  <img src="./doc/r4t_logo.svg" alt="Logo" width="200">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white" alt="Arduino">
  <img src="https://img.shields.io/badge/React-202327?style=for-the-badge&logo=react&logoColor=61DAFB" alt="React">
  <img src="https://img.shields.io/badge/Node.js-339933?style=for-the-badge&logo=nodedotjs&logoColor=white" alt="Node.js">
  <img src="https://img.shields.io/badge/MongoDB-47A248?style=for-the-badge&logo=mongodb&logoColor=white" alt="MongoDB">
  <img src="https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&logo=docker&logoColor=white" alt="Docker">
</p>

---

## Project Overview

### Context
University campus infrastructure typically provides Wi-Fi coverage across most public areas. However, specific strategic locations like the FabLab are situated in "white zones" where network signals are non-existent. This project provides a real-time environmental monitoring solution for these disconnected zones.

### Core Objectives
* **Data Collection**: Gather temperature and humidity metrics from isolated areas.
* **Protocol Bridging**: Transition data from a non-IP protocol (LoRa) to a networked gateway.
* **Edge Security**: Ensure the authenticity of gathered data before it reaches the infrastructure.
* **Integrity**: Provide a secure and immutable storage method to prevent historical data tampering.
* **Real-time Visualization**: Display updates through a centralized dashboard accessible via the campus network.

### Strategic Gains
* **Operational Range**: LoRa modules combined with high-gain antennas allow for transmission over distances that Wi-Fi cannot reach.
* **Energy Autonomy**: Efficient hardware management and FreeRTOS task scheduling ensure independent operation.
* **Data Authenticity**: HMAC signatures guarantee that received data is authorized and unaltered.
* **Auditability**: Blockchain-inspired storage logic in MongoDB creates a verifiable chain of custody.

---

## Tech Stack

| Layer | Technologies |
| :--- | :--- |
| **Edge Hardware** | Arduino UNO R4 WiFi, Grove Wio-E5 (LoRa), DHT-22 |
| **Edge Software** | C++, FreeRTOS, HMAC Cryptography |
| **Infrastructure** | Docker, Mosquitto (MQTT), MongoDB |
| **Backend** | Node.js, Express, WebSockets |
| **Frontend** | React 19, TypeScript, Vite, Tailwind CSS, Recharts |

---

## System Architecture

### Software Design
The system relies on a distributed architecture where the Gateway acts as a translator between the physical LoRa layer and the digital IP layer.

#### Component Schemas
* **Receiver/Gateway**: Manages LoRa reception and Wi-Fi/MQTT bridging.
* **Sender/Node**: Handles sensor acquisition and cryptographic signing.
* **Containerized Services**: Orchestrates the database, broker, and web adapter.

<p align="center">
  <img src="./doc/services_schema.png" width="800" alt="Services Schema" />
</p>

---

## Security Protocol

The system implements a multi-layer security protocol to ensure the integrity and authenticity of environmental data.

### 1. Data Authenticity (HMAC-SHA256)
To prevent data spoofing, every payload sent to the Backend is signed using a Hash-based Message Authentication Code.
* The Gateway generates a signature using a shared secret key and the message content.
* The Adapter recalculates the signature upon reception.
* Mismatched signatures result in immediate packet disposal.

### 2. Anti-Replay Mechanism
To protect against replay attacks, a sequence counter is integrated into the protocol.
* Each message includes a strictly increasing sequence number.
* The Backend maintains a record of the last received counter for each device.
* Messages with outdated sequence numbers are rejected.

### 3. Data Immutability (Blockchain Ledger)
Once validated, data is stored in MongoDB using a structure inspired by blockchain technology.
* **Block Hashing**: Each record contains a cryptographic hash of the entire document.
* **Chaining**: Every new entry includes the hash of the preceding record.
* **Verification**: This creates a verifiable chain; if a historical record is modified, the hash chain is broken.

---

## Installation

### Prerequisites
Ensure the following software is installed:
* Docker & Docker Compose
* Node.js (v20+) & NPM
* Arduino CLI
* Just (Command Runner)

### 1. Hardware Configuration
Before flashing, modify the `config.h` files in both `sender/` and `receiver/` directories:
* Set the `LORA_SHARED_SECRET` for HMAC signing.
* Configure `WIFI_SSID` and `WIFI_PASS` for the Receiver node.

**Install Dependencies:**
```bash
just setup-arduino
```

**Flash Boards:**
```bash
# Identify your ports
just ports

# Flash the Sender
just sender COM*

# Flash the Receiver
just receiver COM*
```

### 2. Infrastructure Setup

**Environment Variables:**
Create a `.env` file in the root directory:
```env
VITE_WS_URL=ws://[YOUR_SERVER_IP]:[PORT]
```

**Launch Services:**
```bash
# Install all node modules
just install

# Start all containers
just up
```

---

## Usage
Navigate to your server IP address on port 80 in a web browser. The dashboard will automatically connect to the WebSocket stream to display live telemetry and historical data.

---
<p align="center"><i>Developed with love 💖 - CESI.</i></p>

