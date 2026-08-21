#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define SOIL_PIN 34
#define LDR_PIN 35
#define DHT_PIN 15
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);


// =================================================
// WIFI + MQTT SETTINGS
// =================================================

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* MQTT_TOPIC = "greensense/status";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define SOIL_LOW_THRESHOLD 500
#define SOIL_HIGH_THRESHOLD 1600

#define LIGHT_LOW_THRESHOLD 500
#define LIGHT_HIGH_THRESHOLD 2000

#define TEMP_THRESHOLD 25.0

#define HUMIDITY_THRESHOLD 50.0


// =================================================
// WIFI CONNECTION
// =================================================

void connectWiFi() {

  Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
}


// =================================================
// MQTT CONNECTION
// =================================================

void connectMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Connecting to MQTT");

    String clientID = "GreenSense-";
    clientID += String(random(0xffff), HEX);

    if (mqttClient.connect(clientID.c_str())) {

      Serial.println("Connected");

    } else {

      Serial.print("Failed, State=");
      Serial.println(mqttClient.state());

      delay(2000);
    }
  }
}


// =================================================
// MQTT PUBLISH
// =================================================

void publishMQTT(
  int soil,
  int light,
  float temperature,
  float humidity,
  String status
) {

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  String message = "{";

  message += "\"soil\":";
  message += soil;

  message += ",\"light\":";
  message += light;

  message += ",\"temperature\":";
  message += temperature;

  message += ",\"humidity\":";
  message += humidity;

  message += ",\"status\":\"";
  message += status;

  message += "\"}";

  mqttClient.publish(MQTT_TOPIC, message.c_str());

  Serial.println();
  Serial.println("MQTT Published:");
  Serial.println(message);
}

void setup() {

  Serial.begin(115200);

  pinMode(SOIL_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);

  dht.begin();

  connectWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  connectMQTT();

  Serial.println();
  Serial.println("GreenSense Started");
}

void loop() {

  int soilValue = analogRead(SOIL_PIN);

  int lightValue = analogRead(LDR_PIN);

  float temperature = dht.readTemperature();

  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {

    Serial.println("DHT22 reading failed!");

    delay(2000);

    return;
  }

  String soilLevel;

  if (soilValue < 500) {

    soilLevel = "LOW";

  }
  else if (soilValue < 1600) {

    soilLevel = "LOW-MID";

  }
  else if (soilValue < 2800) {

    soilLevel = "HIGH-MID";

  }
  else {

    soilLevel = "HIGH";
  }

  bool soilLow = soilValue < SOIL_HIGH_THRESHOLD;

  bool soilHigh = soilValue >= SOIL_HIGH_THRESHOLD;


  String lightLevel;

  if (lightValue < 300) {

    lightLevel = "LOW";

  }
  else if (lightValue < 500) {

    lightLevel = "LOW-MID";

  }
  else if (lightValue < 2000) {

    lightLevel = "HIGH-MID";

  }
  else {

    lightLevel = "HIGH";
  }


  bool lightLow = lightValue < LIGHT_LOW_THRESHOLD;

  bool lightHigh = lightValue >= LIGHT_LOW_THRESHOLD;


  String temperatureLevel;

  if (temperature > TEMP_THRESHOLD) {

    temperatureLevel = "HIGH";

  }
  else {

    temperatureLevel = "LOW";
  }


  bool temperatureHigh = temperature > TEMP_THRESHOLD;

  bool temperatureLow = temperature <= TEMP_THRESHOLD;


  String humidityLevel;

  if (humidity < HUMIDITY_THRESHOLD) {

    humidityLevel = "LOW";

  }
  else {

    humidityLevel = "HIGH";
  }


  bool humidityLow = humidity < HUMIDITY_THRESHOLD;

  bool humidityHigh = humidity >= HUMIDITY_THRESHOLD;


  Serial.println();

  Serial.print("Soil Moisture: ");
  Serial.print(soilValue);
  Serial.print(" : ");
  Serial.println(soilLevel);

  Serial.print("Light: ");
  Serial.print(lightValue);
  Serial.print(" : ");
  Serial.println(lightLevel);

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" C : ");
  Serial.println(temperatureLevel);

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" % : ");
  Serial.println(humidityLevel);


  bool anomaly = false;

  String status = "NORMAL";


  if (soilLow && humidityHigh) {

    anomaly = true;

    status = "HUMID_AIR_DRY_SOIL";

    Serial.println();
    Serial.println("ANOMALY DETECTED");
    Serial.println(
      "Description: Air is humid but the soil is relatively dry."
    );
    Serial.println(
      "Solution: Check irrigation and soil water retention."
    );
  }



  else if (soilHigh && humidityLow) {

    anomaly = true;

    status = "WET_SOIL_DRY_AIR";

    Serial.println();
    Serial.println("ANOMALY DETECTED");
    Serial.println(
      "Description: Soil is wet while the surrounding air is dry."
    );
    Serial.println(
      "Solution: Monitor evaporation and avoid unnecessary watering."
    );
  }


  else if (temperatureHigh && lightLow) {

    anomaly = true;

    status = "HOT_LOW_LIGHT";

    Serial.println();
    Serial.println("ANOMALY DETECTED");
    Serial.println(
      "Description: Temperature is high despite low light conditions."
    );
    Serial.println(
      "Solution: Check ventilation or possible external heat sources."
    );
  }



  else if (temperatureLow && lightHigh) {

    anomaly = true;

    status = "COOL_HIGH_LIGHT";

    Serial.println();
    Serial.println("ANOMALY DETECTED");
    Serial.println(
      "Description: Light intensity is high while temperature remains low."
    );
    Serial.println(
      "Solution: Check for artificial lighting or active cooling."
    );
  }



  if (!anomaly) {

    Serial.println();

    if (soilLow && humidityLow) {

      status = "DRY";

      Serial.println(
        "Inference: Dry conditions."
      );
    }

    else if (soilHigh && humidityHigh) {

      status = "WET_HUMID";

      Serial.println(
        "Inference: Wet and humid conditions."
      );
    }

    else if (temperatureHigh && lightHigh) {

      status = "WARM_WELL_LIT";

      Serial.println(
        "Inference: Warm and well-lit conditions."
      );
    }

    else if (temperatureLow && lightLow) {

      status = "COOL_LOW_LIGHT";

      Serial.println(
        "Inference: Cool and low-light conditions."
      );
    }

    else {

      status = "NORMAL";

      Serial.println(
        "Inference: Normal environmental conditions."
      );
    }
  }


  // =================================================
  // MQTT PUBLISH
  // =================================================

  publishMQTT(
    soilValue,
    lightValue,
    temperature,
    humidity,
    status
  );


  // Keep MQTT connection alive

  mqttClient.loop();


  delay(5000);
}
