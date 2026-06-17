#include <WiFi.h>
#include <Preferences.h>
#include <time.h>
#include "config.h"
#include "wifi_manager.h"
#include "offline_queue.h"
#include "firebase_sync.h"

// ── System states ──────────────────────────────────────────────────────────
enum SystemState {
  STATE_INITIAL_SETUP,
  STATE_READY,
  STATE_SUBMITTING,
  STATE_VOTE_SUCCESS,
  STATE_VOTE_FAILED,
  STATE_COOLDOWN
};

SystemState currentState = STATE_READY;

// ── Per-section candidate selection (-1 = nothing chosen) ─────────────────
int selectedCandidate[NUM_SECTIONS] = { -1, -1, -1, -1 };

// ── Election active state (fetched from Firestore, cached in NVS) ──────────
bool electionActive = false;

void saveActiveState(bool active) {
  Preferences prefs;
  prefs.begin("vtic-state", false);
  prefs.putBool("active", active);
  prefs.end();
}

bool loadActiveState() {
  Preferences prefs;
  prefs.begin("vtic-state", true);
  bool v = prefs.getBool("active", false);
  prefs.end();
  return v;
}

// ── Button table ───────────────────────────────────────────────────────────
struct Button {
  int  pin;
  int  section;    // 0-3, -1 for control
  int  candidate;  // 0-2, -1 for control
  bool isReset;
  bool isConfig;
  bool lastState;
  unsigned long lastDebounce;
};

Button buttons[] = {
  { BTN_S1_1, 0, 0, false, false, HIGH, 0 },
  { BTN_S1_2, 0, 1, false, false, HIGH, 0 },
  { BTN_S1_3, 0, 2, false, false, HIGH, 0 },

  { BTN_S2_1, 1, 0, false, false, HIGH, 0 },
  { BTN_S2_2, 1, 1, false, false, HIGH, 0 },
  { BTN_S2_3, 1, 2, false, false, HIGH, 0 },

  { BTN_S3_1, 2, 0, false, false, HIGH, 0 },
  { BTN_S3_2, 2, 1, false, false, HIGH, 0 },
  { BTN_S3_3, 2, 2, false, false, HIGH, 0 },

  { BTN_S4_1, 3, 0, false, false, HIGH, 0 },
  { BTN_S4_2, 3, 1, false, false, HIGH, 0 },
  { BTN_S4_3, 3, 2, false, false, HIGH, 0 },

  { BTN_RESET,  -1, -1, true,  false, HIGH, 0 },
  { BTN_CONFIG, -1, -1, false, true,  HIGH, 0 },
};
const int TOTAL_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

// ── Module instances ───────────────────────────────────────────────────────
WifiManager   wifiManager;
OfflineQueue  offlineQueue;
FirebaseSync  firebaseSync;

String macAddress;
unsigned long stateChangeTime      = 0;
unsigned long lastTelemetryTime    = 0;
unsigned long configPressStartTime = 0;
bool          configHeld           = false;

// ── RGB LED ────────────────────────────────────────────────────────────────
void setRGB(int r, int g, int b) {
  analogWrite(RGB_RED_PIN,   r);
  analogWrite(RGB_GREEN_PIN, g);
  analogWrite(RGB_BLUE_PIN,  b);
}

void updateLED() {
  switch (currentState) {
    case STATE_INITIAL_SETUP: setRGB(128,   0, 128); break; // Purple
    case STATE_READY:
      setRGB(electionActive ? 0 : 255,
             electionActive ? 255 : 140,
             0); // Green = active, Amber = stopped
      break;
    case STATE_SUBMITTING:   setRGB(255, 255,   0); break; // Yellow
    case STATE_VOTE_SUCCESS: setRGB(  0,   0, 255); break; // Blue
    case STATE_VOTE_FAILED:  setRGB(255,   0,   0); break; // Red
    default: break;
  }
}

// ── Helpers ────────────────────────────────────────────────────────────────
bool allSelected() {
  for (int s = 0; s < NUM_SECTIONS; s++)
    if (selectedCandidate[s] < 0) return false;
  return true;
}

void resetSelections() {
  for (int s = 0; s < NUM_SECTIONS; s++) selectedCandidate[s] = -1;
}

void processOfflineQueue() {
  if (WiFi.status() != WL_CONNECTED) return;
  String voteJson;
  while (offlineQueue.peekNext(voteJson)) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, voteJson) == DeserializationError::Ok) {
      JsonArray arr = doc["s"].as<JsonArray>();
      int sel[NUM_SECTIONS];
      for (int s = 0; s < NUM_SECTIONS; s++) sel[s] = arr[s] | 0;
      if (firebaseSync.uploadVote(sel, macAddress)) {
        offlineQueue.popNext();
      } else {
        break;
      }
    } else {
      offlineQueue.popNext(); // discard corrupt entry
    }
  }
}

void syncElectionState() {
  bool remote = firebaseSync.fetchElectionActive(electionActive);
  if (remote != electionActive) {
    electionActive = remote;
    saveActiveState(electionActive);
    updateLED();
    Serial.printf("Election state synced from Firestore: %s\n",
                  electionActive ? "ACTIVE" : "STOPPED");
  }
}

void sendTelemetry() {
  firebaseSync.uploadTelemetry(
    macAddress, true,
    offlineQueue.getPendingCount(),
    WiFi.localIP().toString(),
    WiFi.SSID()
  );
}

// ── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting VTIC Smart Election Terminal...");

  pinMode(RGB_RED_PIN,   OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN,  OUTPUT);

  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }

  macAddress = WiFi.macAddress();
  macAddress.replace(":", "-");
  macAddress = "VTIC-" + macAddress.substring(9); // e.g. VTIC-AB-CD-EF

  // Load cached election active state (used until Firestore is reachable)
  electionActive = loadActiveState();

  offlineQueue.begin();
  wifiManager.begin();

  if (wifiManager.hasCredentials()) {
    currentState = STATE_READY;
    updateLED();
    Serial.println("Connecting to WiFi...");
    if (wifiManager.connect()) {
      Serial.print("Connected. IP: ");
      Serial.println(WiFi.localIP());
      Serial.println("Dashboard: https://your-app.vercel.app");

      configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST UTC+5:30

      firebaseSync.begin(wifiManager.getFirebaseProject(),
                         wifiManager.getFirebaseApiKey());

      syncElectionState();  // fetch live active state from Firestore
      sendTelemetry();
    } else {
      Serial.println("WiFi failed. Offline mode — using cached election state.");
    }
    updateLED();
  } else {
    Serial.println("No WiFi credentials. Launching setup portal...");
    currentState = STATE_INITIAL_SETUP;
    updateLED();
    wifiManager.startPortal();
    wifiManager.waitForScan();
  }
}

// ── Loop ───────────────────────────────────────────────────────────────────
void loop() {
  if (currentState == STATE_INITIAL_SETUP) {
    wifiManager.handlePortal();
    return;
  }

  unsigned long now = millis();

  // ── Button polling ──────────────────────────────────────────────────────
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    Button& btn = buttons[i];
    int reading = digitalRead(btn.pin);

    if (reading != btn.lastState) btn.lastDebounce = now;

    if ((now - btn.lastDebounce) > BUTTON_DEBOUNCE_MS) {
      // Falling edge: button just pressed
      if (reading == LOW && btn.lastState == HIGH) {

        if (btn.isConfig) {
          configHeld           = true;
          configPressStartTime = now;

        } else if (btn.isReset && currentState == STATE_READY) {
          resetSelections();
          Serial.println("Selections cleared by Reset button.");

        } else if (!btn.isReset && !btn.isConfig
                   && currentState == STATE_READY
                   && electionActive) {
          // Voting button
          selectedCandidate[btn.section] = btn.candidate;
          Serial.printf("Section %d → Candidate %d\n", btn.section, btn.candidate);

          if (allSelected()) {
            currentState    = STATE_SUBMITTING;
            stateChangeTime = now;
            updateLED();
            Serial.println("All sections selected. Submitting ballot...");
          }
        }
      }

      // Rising edge: config button released
      if (reading == HIGH && btn.lastState == LOW && btn.isConfig) {
        configHeld = false;
      }
    }
    btn.lastState = reading;
  }

  // ── Config hold → reset to setup portal ────────────────────────────────
  if (configHeld && (now - configPressStartTime) > CONFIG_HOLD_TIME_MS) {
    Serial.println("Config hold. Resetting WiFi credentials...");
    wifiManager.resetCredentials();
    setRGB(128, 0, 128);
    delay(1000);
    ESP.restart();
  }

  // ── State machine ───────────────────────────────────────────────────────
  switch (currentState) {

    case STATE_SUBMITTING: {
      bool ok = firebaseSync.uploadVote(selectedCandidate, macAddress);
      if (ok) {
        currentState = STATE_VOTE_SUCCESS;
        Serial.println("Ballot uploaded to Firebase.");
      } else {
        offlineQueue.enqueue(selectedCandidate);
        currentState = STATE_VOTE_FAILED;
        Serial.println("Firebase unavailable. Ballot queued offline.");
      }
      updateLED();
      stateChangeTime = now;
      break;
    }

    case STATE_VOTE_SUCCESS:
    case STATE_VOTE_FAILED:
      if (now - stateChangeTime >= SUCCESS_LED_TIME_MS) {
        currentState    = STATE_COOLDOWN;
        stateChangeTime = now;
      }
      break;

    case STATE_COOLDOWN: {
      bool ledOn = ((now - stateChangeTime) / 500) % 2 == 0;
      setRGB(ledOn ? 255 : 0, ledOn ? 165 : 0, 0); // Blinking amber
      if (now - stateChangeTime >= COOLDOWN_TIME_MS) {
        resetSelections();
        currentState = STATE_READY;
        updateLED();
        Serial.println("Cooldown done. Ready for next voter.");
      }
      break;
    }

    case STATE_READY:
      if (now - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryTime = now;
        processOfflineQueue();
        if (WiFi.status() == WL_CONNECTED) {
          sendTelemetry();
          syncElectionState(); // check if admin started/stopped from Vercel
        }
      }
      break;

    default:
      break;
  }
}
