# VTIC Smart Election Terminal

An IoT-based school election system built on the ESP32 microcontroller with a live admin dashboard hosted on Vercel. Votes are stored in Firebase Firestore and results update in real time.

## Features

- 4 configurable election sections (e.g. Head Boy, Head Girl, House Leader, Sports Captain)
- 3 candidates per section — all names editable from the dashboard
- Live vote count bars that update instantly via Firestore real-time listeners
- Start / Stop election from the dashboard — ESP32 terminals respond within 30 s
- Offline vote buffering — ballots saved locally on the ESP32 and synced when WiFi returns
- Multi-terminal support — connect multiple ESP32 units to the same Firebase project
- Device status panel showing each terminal's IP, WiFi SSID, and pending vote count
- Captive portal for WiFi and Firebase setup (no code changes needed on site)

## Architecture

```
Vercel Dashboard  ←──── Firebase Firestore ────→  ESP32 Terminal(s)
  (Next.js app)          (shared backend)           (reads & writes)
```

| Layer | Role |
|---|---|
| **Vercel** | Admin dashboard — edit names, start/stop election, view live results |
| **Firebase Firestore** | Shared database between dashboard and all ESP32 terminals |
| **ESP32** | Handles physical buttons, uploads ballots, polls election state every 30 s |

## Hardware

| Component | Detail |
|---|---|
| ESP32 Development Board | Main controller |
| 12 Push Buttons | 3 per election section × 4 sections |
| 2 Control Buttons | Reset selection, Config (WiFi setup) |
| RGB LED | Status indicator |
| Breadboard / PCB | Wiring |
| USB Power Supply | 5 V |

## Button GPIO Layout

```
Section 1:  GPIO  4, 12, 13      Section 2:  GPIO 14, 15, 16
Section 3:  GPIO 17, 18, 19      Section 4:  GPIO 21, 22, 23

Reset:  GPIO 25
Config: GPIO 26  ← hold 5 s to re-enter WiFi setup
RGB:    GPIO 27 (R)  32 (G)  33 (B)
```

**LED status colours**

| Colour | Meaning |
|---|---|
| Amber (solid) | Election stopped |
| Green (solid) | Election active, ready to vote |
| Yellow | Submitting ballot |
| Blue | Vote uploaded successfully |
| Red | Upload failed, saved offline |
| Amber (blinking) | Cooldown between voters |
| Purple | First-boot WiFi setup mode |

## Voting Flow

1. Admin opens the Vercel dashboard and clicks **Start Election**
2. Voter presses one button per section (up to 4 selections)
3. When all 4 sections are selected the ballot is auto-submitted
4. ESP32 uploads the ballot to Firestore — dashboard updates instantly
5. 10 s cooldown, then the terminal is ready for the next voter

## Project Structure

```
ESP32-Voting-Machine/
├── dashboard/                   ← Deploy to Vercel
│   ├── app/
│   │   ├── layout.js
│   │   ├── page.js              ← Main dashboard UI
│   │   └── globals.css
│   ├── lib/
│   │   └── firebase.js          ← Firebase init
│   ├── package.json
│   ├── next.config.mjs
│   └── .env.example             ← Copy to .env.local, fill in Firebase keys
│
└── firmware/
    └── vtic_voting_machine/
        ├── vtic_voting_machine.ino   ← Main state machine
        ├── config.h                  ← GPIO pins & system constants
        ├── firebase_sync.h           ← Firestore ballot upload & state fetch
        ├── offline_queue.h           ← LittleFS offline ballot buffer
        └── wifi_manager.h            ← Captive portal WiFi setup
```

## Firestore Data Structure

```
config/
  election/
    active:   boolean                    ← toggled from dashboard
    sections: [ { name, candidates[] } ] ← edited from dashboard

votes/
  {auto-id}/
    section0: 0      ← candidate index chosen (0, 1, or 2)
    section1: 2
    section2: 1
    section3: 0
    deviceId: "VTIC-AB-CD-EF"
    timestamp: 1234567890

devices/
  {deviceId}/
    online, lastActive, ipAddress, ssid, pendingVotes
```

## Setup Guide

### 1 — Firebase

1. Go to [console.firebase.google.com](https://console.firebase.google.com) and create a project
2. Enable **Firestore Database** → start in test mode
3. Go to **Project Settings → Your apps → Web app** and copy the config values
4. Set Firestore security rules (Firestore → Rules tab):

```js
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    match /{document=**} {
      allow read, write: if true;
    }
  }
}
```

> Restrict to authenticated users for a production deployment.

### 2 — Vercel Dashboard

**Run locally first:**

```bash
cd dashboard
cp .env.example .env.local    # fill in your Firebase values
npm install
npm run dev                   # → http://localhost:3000
```

**Deploy to Vercel:**

1. Push this repo to GitHub
2. Import the repo at [vercel.com/new](https://vercel.com/new)
3. Set **Root Directory** to `dashboard`
4. Add all `NEXT_PUBLIC_FIREBASE_*` variables under Settings → Environment Variables
5. Click Deploy

### 3 — ESP32 Firmware

1. Open `firmware/vtic_voting_machine/` in Arduino IDE
2. Install required libraries via Library Manager:
   - **ArduinoJson** by Benoit Blanchon
   - **LittleFS** (included with the ESP32 board package)
3. Select your ESP32 board and port, then upload
4. On first boot the ESP32 creates a WiFi access point: **`VTIC-VOTING-SETUP`**
5. Connect to that AP from your phone or laptop
6. Enter your WiFi SSID, password, Firebase Project ID, and API Key → Save
7. The ESP32 restarts and connects — it will appear in the Devices panel on the dashboard

## Software Requirements

- Node.js 18+ (for the dashboard)
- Arduino IDE 2.x
- ESP32 board package for Arduino
- Firebase project (free Spark plan is sufficient)

## Applications

- School and college elections
- Club or society polls
- Event voting
- IoT demonstrations

## License

MIT License

## Author

**Badhusha Shaji**  
Founder, Devmorphix
