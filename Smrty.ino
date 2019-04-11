#include <FS.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <PubSubClient.h>

//#define SEALEVELPRESSURE_HPA:1013.25;
#define MQTT_SOCKET_TIMEOUT 60

struct Config {
  char host[64];
  char port[6];
  char username[64];
  char password[64];
  char groupname[64];
  char fingerprint[64];
};

char temperatureTopic[128];
char pressureTopic[128];
char humidityTopic[128];
long lastMsg = 0;

const char *fingerprint = "E3 9D 8B B4 2C FF 51 DD 75 91 93 3A 7B 54 6A 30 36 7D 09 2D";
const char *configFilename = "/configsmrty.json";
const size_t configCapacity = JSON_OBJECT_SIZE(10) + 310;
bool shouldSaveConfig = false;

Config config;                         // <- global configuration object
Adafruit_BME280 bme;

WiFiManager wifiManager;
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

void loadConfiguration(const char *filename, Config &config);
void saveConfiguration(const char *filename, const Config &config);

void setupSensor() {
  Wire.pins(0, 2);
  Wire.begin();

  if (!bme.begin()) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1);
  }
  delay(100); // let sensor boot up
}

void verifyFingerprint() {
  Serial.print("Connecting to ");
  Serial.println(config.host);
  secureClient.setFingerprint(config.fingerprint);
  int portNumber = atoi(config.port);
  if (!secureClient.connect(config.host, portNumber)) {
    Serial.println("Connection failed. Halting execution.");
    return;
  }
  if (secureClient.verify(config.fingerprint, config.host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived");
  wifiManager.resetSettings();
  ESP.reset();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Smrty-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      Serial.println(clientId);
      mqttClient.subscribe("smrty/control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendData() {
  char temperatureString[6];
  char humidityString[6];
  char pressureString[7];

  dtostrf(bme.readTemperature(), 5, 1, temperatureString);
  dtostrf(bme.readHumidity(), 5, 1, humidityString);
  dtostrf(bme.readPressure()/100.0F, 6, 1, pressureString);

  mqttClient.publish(temperatureTopic, temperatureString);
  mqttClient.publish(pressureTopic, pressureString);
  mqttClient.publish(humidityTopic, humidityString);
}

void saveConfigCallback() {
  shouldSaveConfig = true;
}

void setupWifiManager() {
  WiFiManagerParameter custom_hostname("hostname", "hostname", config.host, 64);
  WiFiManagerParameter custom_port("port", "port", config.port, 6);
  WiFiManagerParameter custom_username("username", "username", config.username, 64);
  WiFiManagerParameter custom_password("password", "password", config.password, 64);
  WiFiManagerParameter custom_groupname("groupname", "groupname", config.groupname, 64);
  WiFiManagerParameter custom_fingerprint("fingerprint", "fingerprint", config.fingerprint, 64);

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_hostname);
  wifiManager.addParameter(&custom_port);
  wifiManager.addParameter(&custom_username);
  wifiManager.addParameter(&custom_password);
  wifiManager.addParameter(&custom_groupname);
  wifiManager.addParameter(&custom_fingerprint);

  if (!wifiManager.autoConnect("AutoConnectAP", "superpassword")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  //read updated parameters
  strcpy(config.host, custom_hostname.getValue());
  strcpy(config.port, custom_port.getValue());
  strcpy(config.username, custom_username.getValue());
  strcpy(config.password, custom_password.getValue());
  strcpy(config.groupname, custom_groupname.getValue());
  strcpy(config.fingerprint, custom_fingerprint.getValue());
}

void setup() {
  Serial.begin(115200);

  Serial.println(F("Starting Smrty Sensor...."));
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.println(F("Loading configuration..."));
  loadConfiguration(configFilename, config);
  Serial.println(F("Starting WifiManager"));
  setupWifiManager();

  if (shouldSaveConfig) {
    Serial.println(F("Saving configuration..."));
    saveConfiguration(configFilename, config);
  }
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println("setup Sensor");
  setupSensor();
  Serial.println("Setup mqtt client");

  verifyFingerprint();

  int portNumber = atoi(config.port);
  mqttClient.setServer(config.host, portNumber);
  mqttClient.setCallback(callback);

  strcpy(temperatureTopic,"smrty/sensor/");
  strcpy(humidityTopic,"smrty/sensor/");
  strcpy(pressureTopic,"smrty/sensor/");

  strcat(temperatureTopic,config.groupname);
  strcat(humidityTopic,config.groupname);
  strcat(pressureTopic,config.groupname);

  strcat(temperatureTopic,"/temperature");
  strcat(pressureTopic,"/pressure");
  strcat(humidityTopic,"/humidity");
}

void loop() {
  if (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Smrty-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      Serial.println(clientId);
      mqttClient.subscribe("smrty/control");
      mqttClient.loop();
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  else {
    long now = millis();
    if (now - lastMsg > 10000) {
      lastMsg = now;
      Serial.println("Publish message.");
      sendData();
    }
  }

  
  
}

// Loads the configuration from a file
void loadConfiguration(const char *filename, Config &config) {
  File file = SPIFFS.open(filename,"r");
  StaticJsonDocument<configCapacity> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  strlcpy(config.port,                  // <- destination
          doc["port"] | "8883",  // <- source
          sizeof(config.port));         // <- destination's capacity
  strlcpy(config.host,                  // <- destination
          doc["hostname"] | "io.adafruit.com",  // <- source
          sizeof(config.host));         // <- destination's capacity
  strlcpy(config.username,                  // <- destination
          doc["username"] | "",  // <- source
          sizeof(config.username));         // <- destination's capacity
  strlcpy(config.password,                  // <- destination
          doc["password"] | "",             // <- source
          sizeof(config.password));         // <- destination's capacity
  strlcpy(config.groupname,                  // <- destination
          doc["groupname"] | "wohnzimmer",  // <- source
          sizeof(config.groupname));         // <- destination's capacity
  strlcpy(config.fingerprint,                  // <- destination
          doc["fingerprint"] | "E3 9D 8B B4 2C FF 51 DD 75 91 93 3A 7B 54 6A 30 36 7D 09 2D",  // <- source
          sizeof(config.fingerprint));         // <- destination's capacity
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}

// Saves the configuration to a file
void saveConfiguration(const char *filename, const Config &config) {
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);

  // Open file for writing
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }
  StaticJsonDocument<configCapacity> doc;

  // Set the values in the document
  doc["hostname"] = config.host;
  doc["port"] = config.port;
  doc["username"] = config.username;
  doc["password"] = config.password;
  doc["groupname"] = config.groupname;
  doc["fingerprint"] = config.fingerprint;
  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}

// Prints the content of a file to the Serial
void printFile(const char *filename) {
  // Open file for reading
  File file = SPIFFS.open(filename,"r");
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}
