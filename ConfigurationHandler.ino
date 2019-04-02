// Loads the configuration from a file
void loadConfiguration(const char *filename, Config &config) {
  // Open file for reading
  File file = SPIFFS.open(filename,"r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<configCapacity> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  config.port = doc["port"] | 8883;
  strlcpy(config.hostname,                  // <- destination
          doc["hostname"] | "io.adafruit.com",  // <- source
          sizeof(config.hostname));         // <- destination's capacity
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
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }
  StaticJsonDocument<configCapacity> doc;

  // Set the values in the document
  doc["hostname"] = config.hostname;
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
  File file = SPIFFS.open(filename);
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
