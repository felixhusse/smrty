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

//#define SEALEVELPRESSURE_HPA:1013.25;
//#define AIO_FINGERPRINT: "77 00 54 2D DA E7 D8 03 27 31 23 99 EB 27 DB CB A5 4C 57 18";

// Our configuration structure.
struct Config {
  char host[64];
  char port[6];
  char username[64];
  char key[64];
  char groupname[64];
};

const char *configFilename = "/configsmrty.json";  // <- SD library uses 8.3 filenames
const size_t configCapacity = JSON_OBJECT_SIZE(10) + 310;
bool shouldSaveConfig = false;

Config config;                         // <- global configuration object
Adafruit_BME280 bme;

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
  
}

void sendData() {
  
}

void saveConfigCallback() {
  shouldSaveConfig = true;
}

void setupWifiManager() {
  WiFiManagerParameter custom_hostname("hostname", "hostname", config.host, 64);
  WiFiManagerParameter custom_port("port", "port", config.port, 6);
  WiFiManagerParameter custom_username("username", "username", config.username, 64);
  WiFiManagerParameter custom_key("key", "key", config.key, 64);
  WiFiManagerParameter custom_groupname("groupname", "groupname", config.groupname, 64);
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_hostname);
  wifiManager.addParameter(&custom_port);
  wifiManager.addParameter(&custom_username);
  wifiManager.addParameter(&custom_key);
  wifiManager.addParameter(&custom_groupname);

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
  strcpy(config.key, custom_key.getValue());
  strcpy(config.groupname, custom_groupname.getValue());
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
}

void loop() {
  // not used in this example
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
  strlcpy(config.key,                  // <- destination
          doc["key"] | "",             // <- source
          sizeof(config.key));         // <- destination's capacity
  strlcpy(config.groupname,                  // <- destination
          doc["groupname"] | "wohnzimmer",  // <- source
          sizeof(config.groupname));         // <- destination's capacity
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
  doc["key"] = config.key;
  doc["groupname"] = config.groupname;

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
