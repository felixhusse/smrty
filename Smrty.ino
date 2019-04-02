#include <FS.h>
#include <ArduinoJson.h>
#include <SPI.h>

#define SEALEVELPRESSURE_HPA:1013.25;
#define AIO_FINGERPRINT: "77 00 54 2D DA E7 D8 03 27 31 23 99 EB 27 DB CB A5 4C 57 18";

// Our configuration structure.
struct Config {
  char hostname[64];
  int port;
  char username[64];
  char key[64];
  char groupname[64];
};
const char *configFilename = "/config.json";  // <- SD library uses 8.3 filenames
const size_t configCapacity = JSON_OBJECT_SIZE(10) + 310;
Config config;                         // <- global configuration object

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
  Serial.println(config.hostname);

  if (! client.connect(config.hostname, config.port)) {
    Serial.println("Connection failed. Halting execution.");
    while(1);
  }

  if (client.verify(AIO_FINGERPRINT, config.hostname)) {
    Serial.println("Connection secure.");
  } else {
    Serial.println("Connection insecure! Halting execution.");
    while(1);
  }
}

void setup() {
  // Initialize serial port
  Serial.begin(9600);
  while (!Serial) continue;

  // Initialize SD library
  while (!SPIFFS.begin()) {
    Serial.println(F("Failed to initialize SD library"));
    delay(1000);
  }

  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  loadConfiguration(filename, config);

  // Create configuration file
  Serial.println(F("Saving configuration..."));
  saveConfiguration(filename, config);

  // Dump config file
  Serial.println(F("Print config file..."));
  printFile(filename);
}

void loop() {
  // not used in this example
}
