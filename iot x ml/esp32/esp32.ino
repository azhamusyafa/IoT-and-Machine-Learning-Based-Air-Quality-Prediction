#include <DHT11.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Definisi Pin
#define MQ135_PIN 34         // Pin Analog untuk MQ-135
#define LAMP_RED_PIN 25      // Pin Digital untuk Lampu Merah
#define LAMP_YELLOW_PIN 26   // Pin Digital untuk Lampu Kuning
#define LAMP_GREEN_PIN 27    // Pin Digital untuk Lampu Hijau
#define Kipas_PIN 14         // Pin Digital untuk Kipas
DHT11 dht(14);

// Wi-Fi dan Firebase credentials
#define WIFI_SSID "Fiaa"
#define WIFI_PASSWORD "123456789"
#define API_KEY "AIzaSyA85f9vvHhhoH4srjh-PSsa_WrdR6dEK6Y"
#define DATABASE_URL "https://fixiotml-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// Function to control LEDs based on sensor values
void updateLEDs(float temperature, int airQuality) {
    // Reset all LEDs first
    digitalWrite(LAMP_GREEN_PIN, LOW);
    digitalWrite(LAMP_YELLOW_PIN, LOW);
    digitalWrite(LAMP_RED_PIN, LOW);
    digitalWrite(Kipas_PIN, LOW);

    // LED logic based on conditions
    if (airQuality > 600) {
        // Red LED condition - Air quality is poor
        digitalWrite(LAMP_RED_PIN, HIGH);
        digitalWrite(Kipas_PIN, HIGH);  // Turn on fan when air quality is poor
    } else if (temperature > 37 && airQuality < 600) {
        // Yellow LED condition - Temperature is high but air quality is okay
        digitalWrite(LAMP_YELLOW_PIN, HIGH);
    } else {
        // Green LED condition - All parameters are good
        digitalWrite(LAMP_GREEN_PIN, HIGH);
    }

    // Update LED status in Firebase
    if (Firebase.ready() && signupOK) {
        Firebase.RTDB.setBool(&fbdo, "/LED_Status/LED_GREEN", digitalRead(LAMP_GREEN_PIN));
        Firebase.RTDB.setBool(&fbdo, "/LED_Status/LED_YELLOW", digitalRead(LAMP_YELLOW_PIN));
        Firebase.RTDB.setBool(&fbdo, "/LED_Status/LED_RED", digitalRead(LAMP_RED_PIN));
        Firebase.RTDB.setBool(&fbdo, "/LED_Status/FAN", digitalRead(Kipas_PIN));
    }
}

void setup() {
    Serial.begin(115200);

    // Inisialisasi Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());

    // Konfigurasi Firebase
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    config.token_status_callback = tokenStatusCallback;

    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase SignUp OK");
        signupOK = true;
    } else {
        Serial.printf("SignUp Error: %s\n", config.signer.signupError.message.c_str());
    }

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // Konfigurasi Pin
    pinMode(LAMP_RED_PIN, OUTPUT);
    pinMode(LAMP_YELLOW_PIN, OUTPUT);
    pinMode(LAMP_GREEN_PIN, OUTPUT);
    pinMode(Kipas_PIN, OUTPUT);

    Serial.println("Sistem Monitoring Kualitas Udara Dimulai.");
}

void loop() {
    // Membaca nilai dari sensor MQ-135
    int airQualityValue = analogRead(MQ135_PIN);
    
    // Membaca suhu dan kelembaban dari DHT
    float suhu = dht.readTemperature();
    float kelembaban = dht.readHumidity();

    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        // Upload sensor data ke Firebase
        Firebase.RTDB.setInt(&fbdo, "/AirQuality", airQualityValue);
        Firebase.RTDB.setFloat(&fbdo, "/Temperature", suhu);
        Firebase.RTDB.setFloat(&fbdo, "/Humidity", kelembaban);

        // Update LEDs based on sensor values
        updateLEDs(suhu, airQualityValue);

        // Print status to Serial
        Serial.print("Air Quality: ");
        Serial.println(airQualityValue);
        Serial.print("Temperature: ");
        Serial.println(suhu);
        Serial.print("Humidity: ");
        Serial.println(kelembaban);
    }

    delay(5000);
}