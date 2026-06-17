#ifndef ELECTION_CONFIG_H
#define ELECTION_CONFIG_H

#include <Preferences.h>
#include <ArduinoJson.h>
#include "config.h"

struct Section {
  String name;
  String candidates[CANDIDATES_PER_SECTION];
};

class ElectionConfig {
public:
  Section  sections[NUM_SECTIONS];
  bool     active;

  ElectionConfig() : active(false) {
    const char* defaultNames[] = { "Head Boy", "Head Girl", "House Leader", "Sports Captain" };
    for (int s = 0; s < NUM_SECTIONS; s++) {
      sections[s].name = defaultNames[s];
      for (int c = 0; c < CANDIDATES_PER_SECTION; c++) {
        sections[s].candidates[c] = "Candidate " + String(c + 1);
      }
    }
  }

  void begin() {
    Preferences prefs;
    prefs.begin("vtic-cfg", true);
    active = prefs.getBool("active", false);
    for (int s = 0; s < NUM_SECTIONS; s++) {
      char key[12];
      snprintf(key, sizeof(key), "sn%d", s);
      sections[s].name = prefs.getString(key, sections[s].name);
      for (int c = 0; c < CANDIDATES_PER_SECTION; c++) {
        snprintf(key, sizeof(key), "s%dc%d", s, c);
        sections[s].candidates[c] = prefs.getString(key, sections[s].candidates[c]);
      }
    }
    prefs.end();
  }

  void save() {
    Preferences prefs;
    prefs.begin("vtic-cfg", false);
    prefs.putBool("active", active);
    for (int s = 0; s < NUM_SECTIONS; s++) {
      char key[12];
      snprintf(key, sizeof(key), "sn%d", s);
      prefs.putString(key, sections[s].name);
      for (int c = 0; c < CANDIDATES_PER_SECTION; c++) {
        snprintf(key, sizeof(key), "s%dc%d", s, c);
        prefs.putString(key, sections[s].candidates[c]);
      }
    }
    prefs.end();
  }

  String toJson() {
    DynamicJsonDocument doc(1024);
    doc["active"] = active;
    JsonArray arr = doc.createNestedArray("sections");
    for (int s = 0; s < NUM_SECTIONS; s++) {
      JsonObject obj  = arr.createNestedObject();
      obj["name"]     = sections[s].name;
      JsonArray cands = obj.createNestedArray("candidates");
      for (int c = 0; c < CANDIDATES_PER_SECTION; c++) {
        cands.add(sections[s].candidates[c]);
      }
    }
    String out;
    serializeJson(doc, out);
    return out;
  }

  bool fromJson(const String& json) {
    DynamicJsonDocument doc(1024);
    if (deserializeJson(doc, json) != DeserializationError::Ok) return false;
    if (doc.containsKey("active")) active = doc["active"].as<bool>();
    if (doc.containsKey("sections")) {
      JsonArray arr = doc["sections"].as<JsonArray>();
      for (int s = 0; s < NUM_SECTIONS && s < (int)arr.size(); s++) {
        sections[s].name = arr[s]["name"].as<String>();
        JsonArray cands  = arr[s]["candidates"].as<JsonArray>();
        for (int c = 0; c < CANDIDATES_PER_SECTION && c < (int)cands.size(); c++) {
          sections[s].candidates[c] = cands[c].as<String>();
        }
      }
    }
    return true;
  }
};

#endif
