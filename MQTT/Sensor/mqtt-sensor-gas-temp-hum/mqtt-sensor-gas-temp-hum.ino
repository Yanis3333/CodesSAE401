#include <DHT11.h>
#include <MQ135.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Déclaration des broches
#define DHTPIN 2 // Broche du capteur DHT11 (GPIO2 ou D4 pour ESP8266)
#define ANALOG_PIN A0 // Broche analogique à laquelle le capteur MQ135 est connecté

// Initialisation des objets
DHT11 dht11(DHTPIN);
MQ135 gasSensor(ANALOG_PIN);
WiFiClient espClient;
PubSubClient client(espClient);

// Configuration du réseau WiFi
const char* ssid = "GRP-1-IOM";
const char* password = "Porygon-Z#1";

// Configuration MQTT
const char* mqtt_server = "192.168.1.50";
const char* mqtt_topic_temp = "/Sensor/Temperature/";
const char* mqtt_topic_humid = "/Sensor/Humidity/";
const char* mqtt_topic_gas = "/Sensor/Gas/";

void setup() {
    // Initialisation de la communication série pour le débogage et l'affichage des données
    Serial.begin(115200);

    // Configuration du client WiFi
    setup_wifi();

    // Configuration du serveur MQTT
    client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connexion au réseau ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connecté");
    Serial.println("Adresse IP: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
    // Boucle de reconnexion au serveur MQTT
    while (!client.connected()) {
        Serial.print("Tentative de connexion au serveur MQTT...");
        if (client.connect("ESP-Gas-Hum-Temp", "j", "j")) {
            Serial.println("connecté");
        } else {
            Serial.print("échec, rc=");
            Serial.print(client.state());
            Serial.println(" réessayez dans 5 secondes");
            delay(5000);
        }
    }
}

void loop() {
    // Vérification de la connexion MQTT
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Lecture de la température et de l'humidité depuis le capteur DHT11
    int temperature = 0;
    int humidity = 0;
    int result = dht11.readTemperatureHumidity(temperature, humidity);

    // Vérification de la lecture du capteur DHT11
    if (result == 0) {
        // Affichage des valeurs sur le moniteur série
        Serial.print("Température: ");
        Serial.print(temperature);
        Serial.print(" °C\tHumidité: ");
        Serial.print(humidity);
        Serial.println(" %");

        // Publication des valeurs de température et d'humidité sur MQTT
        if (!client.connected()) {
        reconnect();
        }
        client.publish(mqtt_topic_temp, String(temperature).c_str());
        if (!client.connected()) {
        reconnect();
        }
        client.publish(mqtt_topic_humid, String(humidity).c_str());
    } else {
        // Affichage du message d'erreur
        Serial.println(DHT11::getErrorString(result));
    }

    // Lecture de la valeur corrigée de CO2 depuis le capteur MQ135
    float ppm = gasSensor.getCorrectedPPM(temperature, humidity)/100;

    // Affichage des valeurs sur le moniteur série
    Serial.print("Valeur CO2 corrigée: ");
    Serial.println(ppm);

    if (!client.connected()) {
        reconnect();
    }
    // Publication de la valeur de CO2 sur MQTT
    client.publish(mqtt_topic_gas, String(ppm).c_str());
    delay(60000);
}
