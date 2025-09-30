"""
dataset_generator.py
Generates a synthetic environmental sensor dataset (temperature, humidity, rain, IR, ultrasonic).
Saves the dataset as a CSV file.
"""

import pandas as pd
import random
from datetime import datetime, timedelta

def generate_dataset(num_rows=1000, filename="environmental_sensor_dataset.csv"):
    # Starting timestamp
    start_time = datetime.now()

    # Create timestamps (hourly intervals)
    timestamps = [start_time + timedelta(hours=i) for i in range(num_rows)]
    timestamps = [ts.strftime("%Y-%m-%d %H:%M:%S") for ts in timestamps]

    # Simulate sensor readings
    data = {
        "timestamp": timestamps,
        "ir_distance_cm": [round(random.uniform(5, 80), 2) for _ in range(num_rows)],
        "ultrasonic_distance_cm": [round(random.uniform(2, 400), 2) for _ in range(num_rows)],
        "temperature_C": [round(random.uniform(15, 35), 2) for _ in range(num_rows)],
        "humidity_percent": [round(random.uniform(30, 90), 2) for _ in range(num_rows)],
        "rain_sensor_value": [int(min(max(512 + random.uniform(-300, 300), 0), 1023)) for _ in range(num_rows)]
    }

    # Create DataFrame
    df = pd.DataFrame(data)

    # Save as CSV
    df.to_csv(filename, index=False)
    print(f"✅ Dataset saved as: {filename} with {num_rows} hourly records.")

    return df

if __name__ == "__main__":
    generate_dataset()
