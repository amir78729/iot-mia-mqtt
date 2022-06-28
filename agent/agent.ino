#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoMqttClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#define LDR_PIN A0
int LEDS_PIN[] = {D1, D2, D3, D4};

String ssid = "Dellink";
String pass = "wtkh-daah-y8bj";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char broker[] = "192.168.1.7";
int        port     = 1883;
String ping_topic = "I1820/main/agent/ping";
String log_topic = "I1820/main/log/send";
String notif_topic = "I1820/main/configuration/request";

const long ping_interval = 10000;
const long log_interval = 5000;

unsigned long previous_ping_millis = 0;
unsigned long previous_log_millis = 0;

StaticJsonDocument<200> doc;

void onMqttMessage(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.println("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  String json = "";

  // use the Stream interface to print the contents
  while (mqttClient.available())
    json += (char)mqttClient.read();

  Serial.println(json);
  deserializeJson(doc, json);
  //if ((doc["agent"] == "nodemcu") && (doc["settings"][0]["name"] == "on")) {
  if (doc["agent"] == "nodemcu") {
    if (doc["device"] == "l1")
      digitalWrite(LEDS_PIN[0], doc["settings"][0]["value"]);
    else if (doc["device"] == "l2")
      digitalWrite(LEDS_PIN[1], doc["settings"][0]["value"]);
    else if (doc["device"] == "l3")
      digitalWrite(LEDS_PIN[2], doc["settings"][0]["value"]);
    else if (doc["device"] == "l4")
      digitalWrite(LEDS_PIN[3], doc["settings"][0]["value"]);
  }
  Serial.println();
}

void send_ping() {
  Serial.println("----------------------------------------------------------------------------------------------");
  Serial.print("[");
  Serial.print(timeClient.getFormattedTime());
  Serial.print("] [PING] (");
  Serial.print(ping_topic);
  Serial.println(")");
  String ping = "{\"id\":\"nodemcu\",\"things\":[{\"id\":\"ldr\",\"type\":\"light\"},{\"id\":\"l1\",\"type\":\"lamp\"},{\"id\":\"l2\",\"type\":\"lamp\"},{\"id\":\"l3\",\"type\":\"lamp\"},{\"id\":\"l4\",\"type\":\"lamp\"}],\"actions\":[]}";
  Serial.println(ping);
  mqttClient.beginMessage(ping_topic);
  mqttClient.print(ping);
  mqttClient.endMessage();
}

void send_log() {
  Serial.println("----------------------------------------------------------------------------------------------");
  Serial.print("[");
  Serial.print(timeClient.getFormattedTime());
  Serial.print("] [LOG] (");
  Serial.print(log_topic);
  Serial.println(")");
  int ldr_value = analogRead(LDR_PIN);
  String ldr_message = "{\"timestamp\":" + String(timeClient.getEpochTime()) + ",\"type\":\"light\",\"device\":\"ldr\",\"states\":[{\"name\":\"light\",\"value\":" + String(map(ldr_value, 0, 1024, 100, 0)) + "}],\"agent\":\"nodemcu\"}";
  Serial.println(ldr_message);
  mqttClient.beginMessage(log_topic);
  mqttClient.print(ldr_message);
  mqttClient.endMessage();
}


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
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
  Serial.println(broker);
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
  mqttClient.onMessage(onMqttMessage);
  mqttClient.subscribe(notif_topic);
  for (unsigned int i = 0; i < sizeof(LEDS_PIN) / sizeof(LEDS_PIN[0]); i++)
    pinMode(LEDS_PIN[i], OUTPUT);
  timeClient.begin();
  timeClient.setTimeOffset(16200); // GMT+4:30 (4.5*3600)
}

void loop() {
  mqttClient.poll();
  timeClient.update();
  unsigned long currentMillis = millis();
  
  if (currentMillis - previous_ping_millis >= ping_interval) {
    previous_ping_millis = currentMillis;
    send_ping();
  }
  
  if (currentMillis - previous_log_millis >= log_interval) {
    previous_log_millis = currentMillis;
    send_log();
  }
}
