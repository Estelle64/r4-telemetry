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
    arduino-cli lib install "PubSubClient"

# Build and start the containers
# ensuring dependencies are installed in images
up:
    docker-compose up -d --build

# Stop the containers
down:
    docker-compose down

# View logs
logs:
    docker-compose logs -f
