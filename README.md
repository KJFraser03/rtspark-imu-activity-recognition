# RT-Spark Real-Time IMU Activity Recognition (MQTT + Random Forest)

An end-to-end embedded IoT and Machine Learning pipeline that streams live 6-axis Inertial Measurement Unit (IMU) telemetry from an **STM32F407ZG (RT-Spark)** microcontroller over USB Serial to a **Mosquitto MQTT** broker, classifying human physical activity in real time using a **Random Forest** model (`99.03%` test accuracy).

## System Architecture

```text
[RT-Spark (STM32F407ZG + ICM-20608)]
        │  JSON IMU Telemetry (115200 baud UART @ 5 Hz)
        ▼
[serial_bridge.py (USB Serial COM11)]
        │  Publishes to MQTT topic: rtspark/sensors
        ▼
[Mosquitto MQTT Broker (10.11.39.197:1883)]
        │  Subscribes to: rtspark/sensors
        ▼
[ml_model.py (Random Forest Classifier - 99.03% Accuracy)]
        ├──► Logs timestamped predictions to predictions_log.csv
        └──► Publishes activity state to MQTT topic: rtspark/predictions

PROJECT LEADER AND DEVELOPER : KEN JHIM VILLANUEVA
project collaborator : KHRISMYKHIELBUSA % JOSHUA ROCAMORA
