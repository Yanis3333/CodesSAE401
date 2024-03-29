#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "GRP-1-IOM";
const char* password = "Porygon-Z#1";
const char* mqtt_server = "192.168.1.50";
const char* mqtt_topic = "/Sensor/Motion/";
const int MOTION_PIN = A0;

WiFiClient espClient;
PubSubClient client(espClient);
int lastMotionState = LOW;
unsigned long lastMotionTime = 0;
const unsigned long MOTION_TIMEOUT = 0.25 * 60 * 1000; // 5 minutes en millisecondes
bool motionMessageSent = false; // Variable pour garder une trace de l'envoi du message "0"
bool motionDetectedPreviously = false; // Variable pour garder une trace de l'état précédent du capteur

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  delay(2000);
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
    Serial.print("");
    if (client.connect("ESP-Motion", "j", "j")) {
      Serial.print("");
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
  
  int motionDetected = (analogRead(MOTION_PIN) > 100) ? HIGH : LOW;
  unsigned long currentTime = millis();

  if (motionDetected != lastMotionState) {
    lastMotionState = motionDetected;
    lastMotionTime = currentTime;
    if (motionDetected == HIGH) {
      Serial.println("Mouvement détecté");
      if (!motionDetectedPreviously) {
        client.publish(mqtt_topic, "1");
        motionDetectedPreviously = true;
      }
    }
  } else if (motionDetected == LOW && currentTime - lastMotionTime >= MOTION_TIMEOUT && !motionMessageSent) {
    Serial.println("Pas de mouvement depuis 5 minutes");
    client.publish(mqtt_topic, "0");
    motionDetectedPreviously = false;
    motionMessageSent = true; // Met à jour la variable pour indiquer que le message "0" a été envoyé
  } else if (motionDetected == HIGH) {
    // Si le mouvement est détecté à nouveau, réinitialiser motionMessageSent
    motionMessageSent = false;
  }
  
  delay(200);
}
