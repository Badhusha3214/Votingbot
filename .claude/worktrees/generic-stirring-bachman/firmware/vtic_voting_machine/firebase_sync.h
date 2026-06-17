#ifndef FIREBASE_SYNC_H
#define FIREBASE_SYNC_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"

class FirebaseSync {
private:
  String projectId;
  String apiKey;
  WiFiClientSecure client;

public:
  FirebaseSync() { client.setInsecure(); }

  void begin(String projId, String key) {
    projectId = projId;
    apiKey    = key;
  }

  // Upload a ballot: selected[s] = candidate index for section s.
  // Stored in Firestore `votes/` collection.
  bool uploadVote(int selected[NUM_SECTIONS], String deviceId) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    String url = "https://firestore.googleapis.com/v1/projects/" + projectId
               + "/databases/(default)/documents/votes?key=" + apiKey;
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<512> doc;
    JsonObject fields = doc.createNestedObject("fields");
    for (int s = 0; s < NUM_SECTIONS; s++) {
      String key = "section" + String(s);
      fields.createNestedObject(key)["integerValue"] = String(selected[s]);
    }
    fields.createNestedObject("deviceId")["stringValue"]   = deviceId;
    fields.createNestedObject("timestamp")["integerValue"] = String(time(nullptr));

    String payload;
    serializeJson(doc, payload);
    int code = http.POST(payload);
    http.end();
    return code == 200 || code == 201;
  }

  // Fetch election active state from Firestore `config/election`.
  // Returns the stored value, or `fallback` on any error.
  bool fetchElectionActive(bool fallback = false) {
    if (WiFi.status() != WL_CONNECTED) return fallback;

    HTTPClient http;
    String url = "https://firestore.googleapis.com/v1/projects/" + projectId
               + "/databases/(default)/documents/config/election?key=" + apiKey;
    http.begin(client, url);
    int code = http.GET();
    if (code != 200) { http.end(); return fallback; }

    String body = http.getString();
    http.end();

    DynamicJsonDocument res(1024);
    if (deserializeJson(res, body) != DeserializationError::Ok) return fallback;

    // Firestore REST format: { "fields": { "active": { "booleanValue": true } } }
    if (res["fields"]["active"]["booleanValue"].is<bool>()) {
      return res["fields"]["active"]["booleanValue"].as<bool>();
    }
    return fallback;
  }

  // Update device telemetry in Firestore `devices/{deviceId}`.
  bool uploadTelemetry(String deviceId, bool online, int pendingCount,
                       String ipAddress, String ssid) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    String patchUrl = "https://firestore.googleapis.com/v1/projects/" + projectId
      + "/databases/(default)/documents/devices/" + deviceId
      + "?updateMask.fieldPaths=online&updateMask.fieldPaths=lastActive"
      + "&updateMask.fieldPaths=pendingVotes&updateMask.fieldPaths=ipAddress"
      + "&updateMask.fieldPaths=ssid&key=" + apiKey;

    http.begin(client, patchUrl);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<400> doc;
    JsonObject fields = doc.createNestedObject("fields");
    fields.createNestedObject("online")["booleanValue"]       = online;
    fields.createNestedObject("pendingVotes")["integerValue"] = String(pendingCount);
    fields.createNestedObject("ipAddress")["stringValue"]     = ipAddress;
    fields.createNestedObject("ssid")["stringValue"]          = ssid;
    fields.createNestedObject("lastActive")["integerValue"]   = String(time(nullptr));

    String payload;
    serializeJson(doc, payload);
    int code = http.sendRequest("PATCH", payload);
    http.end();

    if (code == 404) {
      // Document doesn't exist yet — create it
      String createUrl = "https://firestore.googleapis.com/v1/projects/" + projectId
        + "/databases/(default)/documents/devices?documentId=" + deviceId + "&key=" + apiKey;
      http.begin(client, createUrl);
      http.addHeader("Content-Type", "application/json");
      code = http.POST(payload);
      http.end();
    }

    return code == 200 || code == 201;
  }
};

#endif
