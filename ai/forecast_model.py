"""
forecast_model.py
Trains a Facebook Prophet model on environmental dataset and generates forecasts.
"""

import pandas as pd
import matplotlib.pyplot as plt
from prophet import Prophet

def train_prophet(df, column, forecast_hours=168):
    # Prepare Prophet input
    prophet_df = df[['timestamp', column]].rename(columns={'timestamp': 'ds', column: 'y'})
    
    # Train model
    model = Prophet()
    model.fit(prophet_df)
    
    # Create future dataframe
    future = model.make_future_dataframe(periods=forecast_hours, freq='H')
    forecast = model.predict(future)
    
    # Plot forecast
    fig = model.plot(forecast)
    plt.title(f"{column} Forecast")
    plt.xlabel("Time")
    plt.ylabel(column)
    plt.show()
    
    return forecast

if __name__ == "__main__":
    # Load dataset
    df = pd.read_csv("environmental_sensor_dataset.csv")
    df['timestamp'] = pd.to_datetime(df['timestamp'])

    # Forecast variables
    forecast_humidity = train_prophet(df, 'humidity_percent')
    forecast_temperature = train_prophet(df, 'temperature_C')
    forecast_rain = train_prophet(df, 'rain_sensor_value')

    # Save one of the forecasts
    forecast_temperature.to_csv("environmental_forecast.csv", index=False)
    print("✅ Forecasts generated and saved.")
