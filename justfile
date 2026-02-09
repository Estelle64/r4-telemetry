set shell := ["powershell", "-NoProfile", "-Command"]

FQBN := "arduino:renesas_uno:unor4wifi"
BAUD := "115200"

SENDER_SKETCH := "sender/sender.ino"
RECEIVER_SKETCH := "receiver/receiver.ino"

sender port:
    arduino-cli compile --fqbn {{FQBN}} --upload -p {{port}} {{SENDER_SKETCH}}

receiver port:
    arduino-cli compile --fqbn {{FQBN}} --upload -p {{port}} {{RECEIVER_SKETCH}}

monitor port:
    arduino-cli monitor -p {{port}} --config baudrate={{BAUD}}

ports:
    arduino-cli board list

# Install all dependencies (Node.js and Arduino)
install:
    cd adapter; npm install
    cd frontend; npm install
    just setup-arduino

# Install Arduino core and required libraries
setup-arduino:
    arduino-cli core update-index
    arduino-cli core install arduino:renesas_uno
    arduino-cli lib install "FreeRTOS"
    arduino-cli lib install "DHT sensor library"
    arduino-cli lib install "Adafruit Unified Sensor"
    arduino-cli lib install "ArduinoMqttClient"
    arduino-cli lib install "ArduinoJson"

# Build and start the containers
up:
    docker-compose up -d --build

# Stop and remove containers
down:
    docker-compose down

# Stop containers without removing them
stop:
    docker-compose stop

# Start containers (without build)
start:
    docker-compose start

# Restart all containers
restart:
    docker-compose restart

# Rebuild and restart a specific service (ex: just rebuild adapter)
rebuild service:
    docker-compose up -d --build {{service}}

# View all logs
logs:
    docker-compose logs -f

# View logs for a specific service
logs-of service:
    docker-compose logs -f {{service}}

# Show containers status
status:
    docker-compose ps

# Refresh everything: clean stop, install deps, and rebuild
refresh:
    just down
    just install
    just up
