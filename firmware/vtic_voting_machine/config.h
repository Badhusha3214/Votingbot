#ifndef CONFIG_H
#define CONFIG_H

// --- FIREBASE SETTINGS ---
#define DEFAULT_FIREBASE_PROJECT_ID "vtic-voting-system"
#define DEFAULT_FIREBASE_API_KEY    "YOUR_API_KEY_HERE"

// --- VOTING LAYOUT ---
#define NUM_SECTIONS            4
#define CANDIDATES_PER_SECTION  3

// --- GPIO PIN CONFIGURATIONS (4 sections × 3 buttons = 12 voting buttons) ---
#define BTN_S1_1  4
#define BTN_S1_2  12
#define BTN_S1_3  13

#define BTN_S2_1  14
#define BTN_S2_2  15
#define BTN_S2_3  16

#define BTN_S3_1  17
#define BTN_S3_2  18
#define BTN_S3_3  19

#define BTN_S4_1  21
#define BTN_S4_2  22
#define BTN_S4_3  23

// Control buttons
#define BTN_RESET  25
#define BTN_CONFIG 26

// RGB LED (Common Cathode, active HIGH)
#define RGB_RED_PIN   27
#define RGB_GREEN_PIN 32
#define RGB_BLUE_PIN  33

// --- SYSTEM CONSTANTS ---
#define CONFIG_HOLD_TIME_MS    5000   // Hold config button 5s to enter setup portal
#define SUCCESS_LED_TIME_MS    2000   // 2s success LED
#define COOLDOWN_TIME_MS       10000  // 10s cooldown between voters
#define BUTTON_DEBOUNCE_MS     50     // 50ms button debounce
#define TELEMETRY_INTERVAL_MS  30000  // 30s heartbeat to Firebase

#endif
