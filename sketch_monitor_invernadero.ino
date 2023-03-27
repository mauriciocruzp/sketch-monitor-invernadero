#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>

#define DHTPIN 15
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;

const unsigned long delayTime = 10000;

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "Reemplazar con el nombre de la red de internet"
#define WIFI_PASSWORD "Reemplazar con la contraseña"

#define API_KEY "Reemplazar con la api key de la base de datos firebase"
#define DATABASE_URL "Reemplazar con la url de la base de datos firebase"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  dht.begin();
  if (!bmp.begin()) {
    Serial.println("BMP180 sensor has not been detected");
    while (1) {}
  }

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {

    Serial.println("\n------------------------------");
    Serial.println("\nBMP180");

    float pressureBmp = bmp.readPressure() * 0.000009869;
    float realAltitudeBmp = bmp.readAltitude(101500);

    Serial.print("Pressure = ");
    Serial.print(pressureBmp);
    Serial.println(" atm");

    Serial.print("Real altitude = ");
    Serial.print(realAltitudeBmp);
    Serial.println(" meters");

    Serial.println("\nDHT11");

    float humidityDht = dht.readHumidity();
    float temperatureDht = dht.readTemperature();

    Serial.print("Humidity = ");
    Serial.print(humidityDht);
    Serial.println("%");
    Serial.print("Temperature = ");
    Serial.print(temperatureDht);
    Serial.println(" *C");

    sendDataPrevMillis = millis();

    json.set("temperature", temperatureDht);
    json.set("humidity", humidityDht);
    json.set("pressure", pressureBmp);
    json.set("altitude", realAltitudeBmp);

    if (Firebase.RTDB.setJSON(&fbdo, "/test", &json)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}