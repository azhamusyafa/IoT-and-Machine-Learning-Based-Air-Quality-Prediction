from flask import Flask, render_template, jsonify
import firebase_admin
from firebase_admin import credentials, db
import pickle
import numpy as np
from datetime import datetime
from flask_ngrok import run_with_ngrok

app = Flask(__name__)


# Initialize Firebase Admin
cred = credentials.Certificate("E:\kuliah\iot x ml\ixiotml-firebase-adminsdk-6wgza-d3038d371f.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://fixiotml-default-rtdb.asia-southeast1.firebasedatabase.app/'
})

# Load the ML model
with open('model.pkl', 'rb') as file:   
    model = pickle.load(file)

# Air Quality Thresholds
GOOD_THRESHOLD = 450
MODERATE_THRESHOLD = 600

def get_air_quality_status(value):
    if value <= GOOD_THRESHOLD:
        return "Good", "green"
    elif value <= MODERATE_THRESHOLD:
        return "Moderate", "yellow"
    else:
        return "Bad", "red"

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/get_sensor_data')
def get_sensor_data():
    try:
        # Get real-time data from Firebase
        ref = db.reference('/')
        data = ref.get()
        
        air_quality = data.get('AirQuality', 0)
        status, led_color = get_air_quality_status(air_quality)
        
        # Update LED status in Firebase
        led_status = {
            'LED_GREEN': led_color == 'green',
            'LED_YELLOW': led_color == 'yellow',
            'LED_RED': led_color == 'red',
            'FAN': led_color == 'red'  # Fan turns on when air quality is bad
        }
        
        # Update LED status in Firebase
        db.reference('/LED_Status').set(led_status)
        
        # Make prediction automatically
        features = np.array([[
            data.get('Temperature', 0),
            data.get('Humidity', 0),
            air_quality
        ]])
        prediction = model.predict(features)[0]
        
        return jsonify({
            'success': True,
            'airQuality': air_quality,
            'temperature': data.get('Temperature', 0),
            'humidity': data.get('Humidity', 0),
            'status': status,
            'led_color': led_color,
            'prediction': str(prediction),
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})

if __name__ == '__main__':
    app.run(debug=True)