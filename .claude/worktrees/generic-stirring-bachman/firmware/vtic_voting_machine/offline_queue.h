#ifndef OFFLINE_QUEUE_H
#define OFFLINE_QUEUE_H

#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

class OfflineQueue {
private:
  const char* QUEUE_FILE = "/votes_queue.jsonl";

public:
  bool begin() {
    if (!LittleFS.begin(true)) {
      Serial.println("LittleFS Mount Failed");
      return false;
    }
    return true;
  }

  // selected[s] = candidate index for section s
  void enqueue(int selected[NUM_SECTIONS]) {
    File file = LittleFS.open(QUEUE_FILE, FILE_APPEND);
    if (!file) {
      Serial.println("Failed to open queue file");
      return;
    }
    StaticJsonDocument<256> doc;
    JsonArray arr = doc.createNestedArray("s");
    for (int s = 0; s < NUM_SECTIONS; s++) arr.add(selected[s]);
    doc["ts"] = time(nullptr);
    serializeJson(doc, file);
    file.print("\n");
    file.close();
    Serial.println("Vote saved to offline queue.");
  }

  int getPendingCount() {
    if (!LittleFS.exists(QUEUE_FILE)) return 0;
    File file = LittleFS.open(QUEUE_FILE, FILE_READ);
    if (!file) return 0;
    int count = 0;
    while (file.available()) {
      String line = file.readStringUntil('\n');
      if (line.length() > 5) count++;
    }
    file.close();
    return count;
  }

  bool peekNext(String& outJson) {
    if (!LittleFS.exists(QUEUE_FILE)) return false;
    File file = LittleFS.open(QUEUE_FILE, FILE_READ);
    if (!file) return false;
    while (file.available()) {
      String line = file.readStringUntil('\n');
      line.trim();
      if (line.length() > 5) {
        outJson = line;
        file.close();
        return true;
      }
    }
    file.close();
    return false;
  }

  void popNext() {
    if (!LittleFS.exists(QUEUE_FILE)) return;
    File src  = LittleFS.open(QUEUE_FILE, FILE_READ);
    File tmp  = LittleFS.open("/tmp_q.jsonl", FILE_WRITE);
    if (!src || !tmp) { src.close(); tmp.close(); return; }
    bool skipped = false;
    while (src.available()) {
      String line = src.readStringUntil('\n');
      line.trim();
      if (line.length() > 5) {
        if (!skipped) { skipped = true; continue; }
        tmp.print(line + "\n");
      }
    }
    src.close();
    tmp.close();
    LittleFS.remove(QUEUE_FILE);
    LittleFS.rename("/tmp_q.jsonl", QUEUE_FILE);
  }
};

#endif
