import streamlit as st
import requests
import json
import pandas as pd
import plotly.express as px

st.set_page_config(layout='wide')

SERVER_URL = "http://localhost:8080"

def fetch_data(endpoint):
    try:
        response = requests.get(f"{SERVER_URL}{endpoint}")

        response.raise_for_status()
        if endpoint == '/current':
            return response.text
        return response.json()
    except requests.exceptions.RequestException as e:
        st.error(f"Error fetching data from {endpoint}: {e}")
        return None

st.title("Temperature Monitor")

current_temp = fetch_data('/current')
if current_temp:
    st.metric(label="Current Temperature", value=f"{current_temp} °C")


st.header("All Readings")
all_readings = fetch_data('/all_readings')
if all_readings:
    df = pd.DataFrame(all_readings)
    df['time'] = pd.to_datetime(df['time'])

    col1, col2 = st.columns([0.8, 0.2])

    with col1:
        fig = px.line(df, x='time', y='value', title='Temperature over time')
        st.plotly_chart(fig, use_container_width=True)
    with col2:
        st.dataframe(df)


st.header("Hourly Average")
hourly_average = fetch_data('/hourly_average')
if hourly_average:
    df = pd.DataFrame(hourly_average)
    df['time'] = pd.to_datetime(df['time'])

    col1, col2 = st.columns([0.8, 0.2])

    with col1:
        fig = px.line(df, x='time', y='value', title='Hourly Average Temperature')
        st.plotly_chart(fig, use_container_width=True)
    with col2:
        st.dataframe(df)



st.header("Daily Average")
daily_average = fetch_data('/daily_average')
if daily_average:
    df = pd.DataFrame(daily_average)
    df['time'] = pd.to_datetime(df['time'])

    col1, col2 = st.columns([0.8, 0.2])

    with col1:
        fig = px.line(df, x='time', y='value', title='Daily Average Temperature')
        st.plotly_chart(fig, use_container_width=True)
    with col2:
        st.dataframe(df)
