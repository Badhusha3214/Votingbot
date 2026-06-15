#ifndef VOTE_TRACKER_H
#define VOTE_TRACKER_H

#include <Preferences.h>
#include <ArduinoJson.h>
#include "config.h"

class VoteTracker {
public:
  int votes[NUM_SECTIONS][CANDIDATES_PER_SECTION];

  VoteTracker() {
    memset(votes, 0, sizeof(votes));
  }

  void begin() {
    Preferences prefs;
    prefs.begin("vtic-votes", true);
    for (int s = 0; s < NUM_SECTIONS; s++) {
      for (int c = 0; c < CANDIDATES_PER_SECTION; c++) {
        char key[8];
        snprintf(key, sizeof(key), "v%d_%d", s, c);
        votes[s][c] = prefs.getInt(key, 0);
      }
    }
    prefs.end();
  }

  void save() {
    Preferences prefs;
    prefs.begin("vtic-votes", false);
    for (int s = 0; s < NUM_SECTIONS; s++) {
      for (int c = 0; c < CANDIDATES_PER_SECTION; c++) {
        char key[8];
        snprintf(key, sizeof(key), "v%d_%d", s, c);
        prefs.putInt(key, votes[s][c]);
      }
    }
    prefs.end();
  }

  // Cast a full ballot: selected[s] is the candidate index chosen for section s
  void castBallot(int selected[NUM_SECTIONS]) {
    for (int s = 0; s < NUM_SECTIONS; s++) {
      int c = selected[s];
      if (c >= 0 && c < CANDIDATES_PER_SECTION) {
        votes[s][c]++;
      }
    }
    save();
  }

  // Cast a single vote for one candidate in one section (used from dashboard)
  bool castVote(int section, int candidate) {
    if (section < 0 || section >= NUM_SECTIONS)         return false;
    if (candidate < 0 || candidate >= CANDIDATES_PER_SECTION) return false;
    votes[section][candidate]++;
    save();
    return true;
  }

  void reset() {
    memset(votes, 0, sizeof(votes));
    save();
  }

  int getTotal() {
    int total = 0;
    for (int s = 0; s < NUM_SECTIONS; s++)
      for (int c = 0; c < CANDIDATES_PER_SECTION; c++)
        total += votes[s][c];
    return total;
  }

  // Returns JSON: [[s0c0, s0c1, s0c2], [s1c0, ...], ...]
  String toJson() {
    StaticJsonDocument<512> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int s = 0; s < NUM_SECTIONS; s++) {
      JsonArray row = arr.createNestedArray();
      for (int c = 0; c < CANDIDATES_PER_SECTION; c++) {
        row.add(votes[s][c]);
      }
    }
    String out;
    serializeJson(doc, out);
    return out;
  }
};

#endif
