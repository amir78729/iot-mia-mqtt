#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoMqttClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#define LDR_PIN A0
int LEDS_PIN[] = {D1, D2, D3, D4};

String ssid = "ssid";
String pass = "pass";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char BROKER[] = "192.168.1.2";
int PORT = 1883;
String PING_TOPIC = "I1820/main/agent/ping";
String LOG_TOPIC = "I1820/main/log/send";
String NOTIFICATION_TOPIC = "I1820/main/configuration/request";

const long PING_INTERVAL = 10000;
const long LOG_INTERVAL = 5000;

unsigned long previous_ping_millis = 0;
unsigned long previous_log_millis = 0;

StaticJsonDocument<200> doc;

void handle_mqtt_message(int messageSize) {
  Serial.println("----------------------------------------------------------------------------------------------");
  Serial.print("[");
  Serial.print(timeClient.getFormattedTime());
  Serial.print("] [NOTIFICATION] (");
  Serial.print(mqttClient.messageTopic());
  Serial.println(")");
  String json = "";
  while (mqttClient.available()) json += (char)mqttClient.read();
  Serial.println(json);
  deserializeJson(doc, json);
  if (doc["agent"] == "nodemcu") {
    if (doc["device"] == "l1") digitalWrite(LEDS_PIN[0], doc["settings"][0]["value"]);
    else if (doc["device"] == "l2") digitalWrite(LEDS_PIN[1], doc["settings"][0]["value"]);
    else if (doc["device"] == "l3") digitalWrite(LEDS_PIN[2], doc["settings"][0]["value"]);
    else if (doc["device"] == "l4") digitalWrite(LEDS_PIN[3], doc["settings"][0]["value"]);
  }
}

void send_ping() {
  Serial.println("----------------------------------------------------------------------------------------------");
  Serial.print("[");
  Serial.print(timeClient.getFormattedTime());
  Serial.print("] [PING] (");
  Serial.print(PING_TOPIC);
  Serial.println(")");
  String ping = "{\"id\":\"nodemcu\",\"things\":[{\"id\":\"ldr\",\"type\":\"light\"},{\"id\":\"l1\",\"type\":\"lamp\"},{\"id\":\"l2\",\"type\":\"lamp\"},{\"id\":\"l3\",\"type\":\"lamp\"},{\"id\":\"l4\",\"type\":\"lamp\"}],\"actions\":[]}";
  Serial.println(ping);
  mqttClient.beginMessage(PING_TOPIC);
  mqttClient.print(ping);
  mqttClient.endMessage();
}

void send_log() {
  Serial.println("----------------------------------------------------------------------------------------------");
  Serial.print("[");
  Serial.print(timeClient.getFormattedTime());
  Serial.print("] [LOG] (");
  Serial.print(LOG_TOPIC);
  Serial.println(")");
  int ldr_value = analogRead(LDR_PIN);
  String ldr_message = "{\"timestamp\":" + String(timeClient.getEpochTime()) + ",\"type\":\"light\",\"device\":\"ldr\",\"states\":[{\"name\":\"light\",\"value\":" + String(map(ldr_value, 0, 1024, 100, 0)) + "}],\"agent\":\"nodemcu\"}";
  Serial.println(ldr_message);
  mqttClient.beginMessage(LOG_TOPIC);
  mqttClient.print(ldr_message);
  mqttClient.endMessage();
}


void setup() {
  //Initialize serial and wait for PORT to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial PORT to connect. Needed for native USB PORT only
  }
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.print(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println();
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(BROKER);
  if (!mqttClient.connect(BROKER, PORT)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
  mqttClient.onMessage(handle_mqtt_message);
  mqttClient.subscribe(NOTIFICATION_TOPIC);
  for (unsigned int i = 0; i < sizeof(LEDS_PIN) / sizeof(LEDS_PIN[0]); i++)
    pinMode(LEDS_PIN[i], OUTPUT);
  timeClient.begin();
  timeClient.setTimeOffset(16200); // GMT+4:30 (4.5*3600)
}

void loop() {
  mqttClient.poll();
  timeClient.update();
  unsigned long current_millis = millis();

  if (current_millis - previous_ping_millis >= PING_INTERVAL) {
    previous_ping_millis = current_millis;
    send_ping();
  }

  if (current_millis - previous_log_millis >= LOG_INTERVAL) {
    previous_log_millis = current_millis;
    send_log();
  }
}
