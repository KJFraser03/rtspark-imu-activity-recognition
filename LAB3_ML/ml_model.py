import paho.mqtt.client as mqtt
import pandas as pd
import json
import csv
import os
from datetime import datetime
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split

# 1. Load and prepare the dataset
print("Loading dataset.csv...")
dataset = pd.read_csv("dataset.csv")

features = ['acceleration_x', 'acceleration_y', 'acceleration_z', 'gyro_x', 'gyro_y', 'gyro_z']
X = dataset[features]
y = dataset['activity']

# 2. Train the Random Forest Classifier
print("Training the Machine Learning model...")
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
model = RandomForestClassifier(n_estimators=50, random_state=42)
model.fit(X_train, y_train)

accuracy = model.score(X_test, y_test)
print(f"Model trained successfully! Accuracy: {accuracy * 100:.2f}%")

# 3. Prepare CSV Log File for Lab Report
log_file = "predictions_log.csv"
if not os.path.exists(log_file):
    with open(log_file, mode="w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "ax", "ay", "az", "gx", "gy", "gz", "predicted_class", "activity_label"])

# 4. Setup MQTT (Subscribe to sensors, Publish predictions)
broker_address = "10.11.39.197"
sub_topic = "rtspark/sensors"
pub_topic = "rtspark/predictions"

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"Connected to Mosquitto! Listening on '{sub_topic}' & publishing to '{pub_topic}'...")
        client.subscribe(sub_topic)
    else:
        print(f"Connection failed with code {reason_code}")

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        live_sample = pd.DataFrame([[
            data['ax'], data['ay'], data['az'],
            data['gx'], data['gy'], data['gz']
        ]], columns=features)

        prediction = int(model.predict(live_sample)[0])
        label = "Running / Active" if prediction == 1 else "Walking / Rest"
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

        # Log to CSV
        with open(log_file, mode="a", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([
                timestamp, data['ax'], data['ay'], data['az'],
                data['gx'], data['gy'], data['gz'], prediction, label
            ])

        # Publish classification back to MQTT
        result_payload = json.dumps({
            "timestamp": timestamp,
            "prediction": prediction,
            "activity": label
        })
        client.publish(pub_topic, result_payload)

        print(f"[{timestamp}] Logged & Published -> {prediction} ({label})")
    except Exception as e:
        print(f"Parse note: {e}")

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

client.connect(broker_address, 1883)
client.loop_forever()