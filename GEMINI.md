# IoT Project: LoRa Telemetry System

## Project Overview
This project consists of an IoT telemetry system with two main components communicating via LoRa (Long Range):

1.  **Emitter Node (Sensor Node):** 
    - Captures environmental data (Temperature & Humidity).
    - Transmits data packets via LoRa to the Gateway.
    - (Source code not currently in receiver/ context).

2.  **Receiver Node (Gateway):** 
    - **Platform:** Arduino R4 WiFi.
    - **Role:** Centralizes data from the remote node and its own local sensors.
    - **Connectivity:** Connects to WiFi to host a web dashboard.
    - **Interface:** Provides a real-time web interface for users to monitor local and remote environmental data.

---

## Receiver Gateway Architecture (receiver/)
The Gateway software is built on **FreeRTOS** to manage concurrent operations efficiently.

### Directory Structure
- **`receiver.ino`**: Main entry point. Initializes `Serial`, `DataManager`, and creates FreeRTOS tasks (`WiFiTask`, `LoRaTask`, `SensorTask`).

### Tasks (FreeRTOS)
The system runs three main parallel tasks:

1.  **`loraTask`** (`src/tasks/lora_task.cpp`):
    - Manages the LoRa module (via UART `Serial1` using AT commands).
    - Configured for **868MHz**, SF7, BW125.
    - Listens for incoming packets (Format: `+TEST: RX "..."`).
    - Decodes custom hex packets containing `sensor_id`, `data_type`, and `value`.
    - Updates the shared state in `DataManager`.

2.  **`wifiTask`** (`src/tasks/wifi_task.cpp`):
    - Manages WiFi connection (reconnect logic).
    - Controls the **LED Matrix** (displays scrolling text/animations during connection attempts).
    - **Web Server:**
        - Serves the Dashboard HTML (`/`).
        - Serves JSON data (`/api/data`) for the frontend.

3.  **`sensorTask`** (`src/tasks/sensor_task.cpp`):
    - Periodically reads the local **DHT22** sensor.
    - Protected by critical sections to ensure accurate timing during sensor reading.
    - Updates local data in `DataManager`.

### Data Management & Utils
- **`DataManager`** (`src/utils/data_manager.cpp`):
    - Thread-safe singleton using **Semaphores**.
    - Stores the "System State": Local Temp/Hum, Remote Temp/Hum, Last Update Time, Module Statuses (LoRa/DHT).
- **`LedMatrixManager`** (`src/utils/led_matrix_manager.cpp`):
    - Helper to drive the Arduino R4 WiFi LED Matrix.
    - Displays temperature or connection status visually.

### Web Interface
- **`web_assets.h`** (`src/web/web_assets.h`):
    - Contains the entire frontend code stored in program memory (`PROGMEM`).
    - **Tech Stack:** Simple HTML5, **Pico.css** (Styling), **Alpine.js** (Reactive UI).
    - Fetches data every 2 seconds via `/api/data`.

### Hardware Configuration (config.h)
- **LoRa Module:** UART (`Serial1`), Pins: SS(10), RST(9), DIO0(2).
- **Sensor:** DHT22 on Pin 4.
- **WiFi:** Credentials configured in macros.
