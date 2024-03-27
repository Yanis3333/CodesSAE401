#include <MQ135.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Remplacez les informations suivantes par celles de votre réseau WiFi et MQTT
const char* ssid = "GRP-1-IOM";
const char* password = "Porygon-Z#1";
const char* mqtt_server = "192.168.1.50";
const char* mqtt_topic = "/Sensor/Gas/";

const int ANALOG_PIN = A0; // Broche analogique à laquelle le capteur MQ135 est connecté
MQ135 gasSensor = MQ135(ANALOG_PIN);

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Lecture de la valeur de CO2
  float ppm = gasSensor.getPPM();
  
  // Conversion float en chaîne de caractères pour l'envoi via MQTT
  char msg[20];
  dtostrf(ppm/100, 4, 1, msg); // Formatage en chaîne avec 1 décimale
  Serial.print("/Sensor/Gas/");
  Serial.println(ppm/100);
  
  // Envoi de la valeur via MQTT
  client.publish(mqtt_topic, msg);
  
  delay(5000); // Attente de 5 secondes entre chaque envoi de valeur
}
