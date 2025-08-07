
# AI-WalkingStick-System

A smart, AI-powered walking stick designed for visually impaired users. It provides real-time obstacle detection, environmental alerts, GPS tracking, and voice-guided feedback through an embedded system of sensors and modules.

---

## 🚀 Features

- Obstacle detection using Ultrasonic, IR, and Rain sensors
- AI-based obstacle classification and environmental prediction
- Voice alerts using DFPlayer Mini and speaker module
- GPS tracking with NEO-6M module
- Emergency alert via SIM800L
- Vibration feedback and LCD display integration

---

## 📦 Hardware Components

- Arduino Mega 2560
- Ultrasonic Sensor (HC-SR04)
- IR Sensor
- DHT11 (Temperature & Humidity)
- Rain Sensor (MH series)
- DFPlayer Mini with MicroSD
- NEO-6M GPS Module
- SIM800L GSM Module
- LCD Display (parallel)
- Buzzer, Vibration Motor, Panic Button
- Rechargeable Battery + Charging Circuit

---

## 🧠 AI/ML Integration

- Obstacle classification using IR, Ultrasonic & Rain sensors
- Environmental prediction using temperature, humidity & rainfall (lightweight ML logic)
- Real-time decision-making embedded in Arduino code

---

## 🔧 How to Use

1. Clone this repository
2. Open the `.ino` files in Arduino IDE
3. Connect hardware as per provided schematic
4. Upload code to Arduino Mega
5. Power the system and observe feedback through voice, vibration, and display

---

## 🗂️ Folder Structure

```
AI-WalkingStick-System/
│
├── core/               # Pin setup and system boot
├── sensors/            # Sensor reading modules
├── ai/                 # AI logic for classification/prediction
├── alerts/             # Voice and vibration feedback
├── gps/                # GPS tracking logic
├── emergency/          # Panic button and GSM alerts
├── utils/              # Helper functions
└── main.ino            # Central project code
```

---

## 📸 Demo

*Coming soon – images and videos of the system in action*

---

## 🙏 Acknowledgements

Special thanks to my project supervisor, teammates, and all contributors who made this project possible.

---

## 📄 License

This project is licensed under the MIT License.
