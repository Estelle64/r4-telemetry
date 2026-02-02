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
