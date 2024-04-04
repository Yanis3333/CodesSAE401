static const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDXzCCAkegAwIBAgIUUqVmTigzEdD+eM3cbSmzJSAjDVYwDQYJKoZIhvcNAQEL
BQAwPzELMAkGA1UEBhMCRlIxDDAKBgNVBAgMA0JGQzEUMBIGA1UEBwwLTW9udGJl
bGlhcmQxDDAKBgNVBAoMA0lVVDAeFw0yNDA0MDIwODUwNTBaFw0yOTA0MDIwODUw
NTBaMD8xCzAJBgNVBAYTAkZSMQwwCgYDVQQIDANCRkMxFDASBgNVBAcMC01vbnRi
ZWxpYXJkMQwwCgYDVQQKDANJVVQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQCyB9umJs1hPCWJ3ARI2XN8R+dBYuLK8HPV+NRTt9AtOMh98GOddvc7OQFK
lHYERBRksKotzLvV9PPMKsqP+1cfyOUSAj/yPVgIJoBkCIsT5yfWtYlX2ZpIp11G
MwaGvG2j8ws28IZXwfMyGwYQ1eMT9pFY3ukcKAC/nLs9/sk5ja9LXLUaaKQVJD9r
elBaqLAEFFE8bmJi9gQNqXGnO5w2e8veemP7d1Sgv3qyjNM+aJRhq8Kq+1GlM4Hs
Hgt/4tCJdaaWfkv9IoJy9HbKOlg3kNB2MGab2CZCR0dMRuWeFOtgKW6dAvRx8E/h
M40oNqe1ZbZARwOeQHZckKPzF9sxAgMBAAGjUzBRMB0GA1UdDgQWBBR5BM1A0mzb
M4ZYXX76by6HHoN9CjAfBgNVHSMEGDAWgBR5BM1A0mzbM4ZYXX76by6HHoN9CjAP
BgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAUMXELqYKkawUiPwrU
DafNbBnggWUGAuU4oiQe8dAGk6/usLq5hx64E4SU3jECRLiK2ifzE2wlEMBeVv2e
QGZIeL6k7JTDFuS+fTvokD3SQVn5qy0VhalqQF3dmtvg7m+c6I50ZMVEBsXdniMB
tA3XTyJeSaGWGtN2SMhIYrhJjpx/NcsLDqfh07B9txKJaKtfMiMlGI5Z/3PQ/7gr
ohxPIjHvomi03C/YA7PP1CuZdzerT9z6WReXtRcG40ddsspKg6MqnxeybtP+qoT2
POZe/GCOAGGsJS6MpLYmsM3XzG3kEJggJrpAQWUvl59b9m9G1ePJ+FfE/oHyb4PE
H6gS
-----END CERTIFICATE-----
)EOF";

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "GRP-1-IOM";
const char* password = "Porygon-Z#1";
const char* MqttUser = "GRP-1-IOM";
const char* MqttPass = "Porygon-Z#1";
const char* mqtt_server = "192.168.1.50";
const int MOTION_PIN = A0;
const char* mqtt_topic = "/Sensor/Motion/";

BearSSL::WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
int lastMotionState = LOW;
unsigned long lastMotionTime = 0;
const unsigned long MOTION_TIMEOUT = 5 * 60 * 1000; // 5 minutes en millisecondes
bool motionMessageSent = false; // Variable pour garder une trace de l'envoi du message "0"
bool motionDetectedPreviously = false; // Variable pour garder une trace de l'état précédent du capteur
bool lastPublishedState = false; // Variable pour garder une trace du dernier état publié
unsigned long lastChangeTime = 0; // Variable pour garder une trace du dernier changement d'état

void setClock()
{
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

void reconnect() {
  char err_buf[256];
  
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), MqttUser, MqttPass)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      espClient.getLastSSLError(err_buf, sizeof(err_buf));
      Serial.print("SSL error: ");
      Serial.println(err_buf);
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  BearSSL::X509List *serverTrustedCA = new BearSSL::X509List(ca_cert);
  espClient.setTrustAnchors(serverTrustedCA);
  espClient.setInsecure();
  setup_wifi();
  setClock(); // Required for X.509 validation
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);
  pinMode(MOTION_PIN, INPUT);
}


void loop() {

  if (!client.connected()) {
    reconnect();
  }
  delay(100);
  
  int motionDetected = (analogRead(MOTION_PIN) > 900) ? HIGH : LOW;
  unsigned long currentTime = millis();
  Serial.print("Motion Value : ");
  Serial.println(analogRead(MOTION_PIN));

  if (motionDetected != lastMotionState) {
    lastMotionState = motionDetected;
    lastMotionTime = currentTime;
    if (motionDetected == HIGH) {
      Serial.println("Mouvement détecté");
      if (!motionDetectedPreviously) {
        client.loop();
        client.publish(mqtt_topic, "1");
        motionDetectedPreviously = true;
      }
    }
  } else if (motionDetected == LOW && currentTime - lastMotionTime >= MOTION_TIMEOUT && !motionMessageSent) {
    Serial.println("Pas de mouvement depuis 5 minutes");
    client.loop();
    client.publish(mqtt_topic, "0");
    motionDetectedPreviously = false;
    motionMessageSent = true; // Met à jour la variable pour indiquer que le message "0" a été envoyé
  } else if (motionDetected == HIGH && currentTime - lastMotionTime >= MOTION_TIMEOUT) {
    Serial.println("Du mouvement depuis 5 minutes");
    client.loop();
    client.publish(mqtt_topic, "1");
    lastMotionTime = currentTime;
  } else if (motionDetected == HIGH) {
    // Si le mouvement est détecté à nouveau, réinitialiser motionMessageSent
    motionMessageSent = false;
  }
  
  delay(100);
}
