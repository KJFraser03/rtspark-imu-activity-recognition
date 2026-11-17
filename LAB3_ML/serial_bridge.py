import serial
import paho.mqtt.client as mqtt
import json

COM_PORT = "COM12"  # <-- Change this to your RT-Spark's COM port
BAUD_RATE = 115200
BROKER_ADDRESS = "10.11.34.133"
TOPIC = "rtspark/sensors"

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.connect(BROKER_ADDRESS, 1883)
client.loop_start()

print(f"Reading live RT-Spark IMU from {COM_PORT} and publishing to '{TOPIC}'...\n")

try:
    with serial.Serial(COM_PORT, BAUD_RATE, timeout=1) as ser:
        while True:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if line.startswith("{") and line.endswith("}"):
                # Validate JSON before publishing
                data = json.loads(line)
                client.publish(TOPIC, json.dumps(data))
                print(f"Published Live Hardware Data -> {line}")
except KeyboardInterrupt:
    print("\nBridge stopped.")
finally:
    client.loop_stop()
    client.disconnect()