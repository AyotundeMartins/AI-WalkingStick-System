
# 👨‍🦯 AI-WalkingStick-System

A smart, AI-powered walking stick designed for visually impaired users. It provides **real-time obstacle detection**, **environmental alerts**, **GPS tracking**, and **voice-guided feedback** through an embedded system of sensors and modules.

---

## ✨ Features
- Obstacle detection using **Ultrasonic, IR, and Rain sensors**
- AI-based **obstacle classification** and **environmental prediction**
- Voice alerts using **DFPlayer Mini + speaker module**
- GPS tracking with **NEO-6M module**
- Emergency alert via **SIM800L**
- Vibration feedback and **LCD display integration**

---

## 📐 Schematic Diagrams
Circuit and PCB designs are available in the [`docs/Schematic`](./docs/Schematic) folder.

Example schematic:  
![Walking Stick Schematic](./docs/Schematic/Schematic_AI-WAKING-STICK.png)

---

## 🧠 AI Models
The **AI models** are located in the [`ai/`](./ai/) folder.

- `dataset_generator.py` → Generates synthetic environmental dataset (temperature, humidity, rainfall).  
- `forecast_model.py` → Uses **Facebook Prophet** to train and predict environmental conditions.  
- `requirements.txt` → Python dependencies.

### Example Training (Prophet)
```bash
pip install -r ai/requirements.txt
python ai/dataset_generator.py
python ai/forecast_model.py
````

**Output Files**

* `environmental_sensor_dataset.csv` → Generated dataset
* `environmenetal_forecast.csv` → Predicted environmental conditions

---

## 📂 Repository Structure

```
AI-WalkingStick-System/
│── ai/                # Python AI models (dataset + prediction)
│   ├── dataset_generator.py
│   ├── forecast_model.py
│   └── requirements.txt
│
│── docs/Schematic/    # Circuit schematics and PCB layouts
│   ├── PCB_PCB_AI-POWER-WAKING-STICK.brd
│   ├── PCB_PCB_AI-WAKING-STICK.sch
│   ├── Schematic_AI-POWER-WAKING-STICK.png
│   └── Schematic_AI-WAKING-STICK.png
│
│── main.ino           # Arduino main control logic
│── README.md          # Documentation
```

---

Hardware Components

* Arduino Mega 2560
* Ultrasonic Sensor (**HC-SR04**)
* IR Sensor
* DHT11 (**Temperature & Humidity**)
* MH Series Rain Sensor
* DFPlayer Mini + Speaker
* NEO-6M GPS
* SIM800L GSM Module
* Panic Button + Vibration Motor
* LCD Display (parallel interface)

---

Voice Alerts

The DFPlayer Mini SD card contains:

* `0001-High temperature detected`
* `0002-Rainfall likely`
* `0003-Large object detected`
* `0004-Small object detected`
* `0005-Ground is slippery`

---

How to Run

Arduino

1. Open `main.ino` in Arduino IDE.
2. Select **Arduino Mega 2560** board.
3. Upload the code to your device.

Python AI


cd ai
pip install -r requirements.txt
python dataset_generator.py
python forecast_model.py


---

## 👤 Author

Ogunkunle Ayotunde Martins
University of Ilorin, Department of Computer Engineering

---


