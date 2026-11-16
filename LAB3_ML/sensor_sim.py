import paho.mqtt.client as mqtt
import pandas as pd
import json
import time

broker_address = "10.11.39.197"
topic = "rtspark/sensors"

# Load real rows from your dataset to simulate live RT-Spark packets
df = pd.read_csv("dataset.csv")
walk_samples = df[df["activity"] == 0].sample(10, random_state=1)
run_samples = df[df["activity"] == 1].sample(10, random_state=1)
test_stream = pd.concat([walk_samples, run_samples])

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.connect(broker_address, 1883)
client.loop_start()

print(f"Simulating RT-Spark sensor stream to {broker_address} on '{topic}'...\n")

try:
    for _, row in test_stream.iterrows():
        payload = {
            "ax": round(float(row["acceleration_x"]), 4),
            "ay": round(float(row["acceleration_y"]), 4),
            "az": round(float(row["acceleration_z"]), 4),
            "gx": round(float(row["gyro_x"]), 4),
            "gy": round(float(row["gyro_y"]), 4),
            "gz": round(float(row["gyro_z"]), 4)
        }
        client.publish(topic, json.dumps(payload))
        print(f"Sent (True Activity={int(row['activity'])}): {json.dumps(payload)}")
        time.sleep(0.5)
except KeyboardInterrupt:
    pass
finally:
    client.loop_stop()
    client.disconnect()
    print("\nSimulation finished.")