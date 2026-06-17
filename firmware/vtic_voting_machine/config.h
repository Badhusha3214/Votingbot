#ifndef CONFIG_H
#define CONFIG_H

// --- FIREBASE SETTINGS ---
// Enter your Firebase project ID and API key via the WiFi setup portal.
// These defaults are intentionally blank — do not commit real credentials here.
#define DEFAULT_FIREBASE_PROJECT_ID ""
#define DEFAULT_FIREBASE_API_KEY    ""

// --- VOTING LAYOUT ---
// Sections 1–4: 3 candidates each | Sections 5–6: 2 candidates each
// MAX_CANDIDATES_PER_SECTION is the array bound used throughout the firmware.
#define NUM_SECTIONS                6
#define MAX_CANDIDATES_PER_SECTION  3

// --- GPIO PIN CONFIGURATIONS ---
// Section 1 — 3 candidates
#define BTN_S1_1  4
#define BTN_S1_2  12
#define BTN_S1_3  13

// Section 2 — 3 candidates
#define BTN_S2_1  14
#define BTN_S2_2  15
#define BTN_S2_3  16

// Section 3 — 3 candidates
#define BTN_S3_1  17
#define BTN_S3_2  18
#define BTN_S3_3  19

// Section 4 — 3 candidates
#define BTN_S4_1  21
#define BTN_S4_2  22
#define BTN_S4_3  23

// Section 5 — 2 candidates
#define BTN_S5_1  25
#define BTN_S5_2  26

// Section 6 — 2 candidates
#define BTN_S6_1  32
#define BTN_S6_2  33

// Control buttons (input-only pins — add 10kΩ pull-up to 3.3V)
#define BTN_RESET  34
#define BTN_CONFIG 35

// Submit button — voter presses after selecting one candidate per section
#define BTN_SUBMIT 5

// ARGB LED (WS2812B / NeoPixel)
#define ARGB_PIN  27

// Buzzer (active buzzer or piezo, driven HIGH to sound)
#define BUZZER_PIN  2

// --- SYSTEM CONSTANTS ---
#define CONFIG_HOLD_TIME_MS    5000   // Hold config button 5s to enter setup portal
#define SUCCESS_LED_TIME_MS    2000   // 2s success LED
#define COOLDOWN_TIME_MS       10000  // 10s cooldown between voters
#define BUTTON_DEBOUNCE_MS     50     // 50ms button debounce
#define TELEMETRY_INTERVAL_MS  30000  // 30s heartbeat to Firebase

#endif
