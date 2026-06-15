#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include "config.h"

class WifiManager {
private:
  WebServer server;
  DNSServer dnsServer;
  Preferences prefs;
  bool configMode;

  String ssid;
  String password;
  String firebaseProject;
  String firebaseApiKey;

  const byte DNS_PORT = 53;

  void handleRoot() {
    // Build network options from last scan result
    int n = WiFi.scanComplete();
    String options = "<option value=''>-- Select network --</option>";
    if (n > 0) {
      for (int i = 0; i < n; i++) {
        String lock = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " 🔓" : " 🔒";
        options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + lock + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
      }
    }

    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>VTIC Terminal Setup</title>";
    html += "<style>body{font-family:sans-serif;background:#f1f5f9;margin:0;padding:20px;display:flex;justify-content:center;}";
    html += ".card{background:#fff;padding:24px;border-radius:10px;box-shadow:0 4px 6px rgba(0,0,0,0.05);max-width:400px;width:100%;}";
    html += "h2{margin-top:0;color:#1e293b;}";
    html += "label{display:block;margin-top:12px;font-size:13px;color:#475569;}";
    html += "input,select{width:100%;padding:10px;margin:4px 0 0;border:1px solid #cbd5e1;border-radius:6px;box-sizing:border-box;font-size:14px;}";
    html += ".row{display:flex;gap:8px;margin-top:4px;}";
    html += ".row input{margin:0;}";
    html += "button{width:100%;padding:12px;background:#1d4ed8;color:#fff;border:none;border-radius:6px;font-weight:bold;cursor:pointer;margin-top:20px;font-size:15px;}";
    html += "button:hover{background:#1e40af;}";
    html += ".refresh{background:#475569;margin-top:8px;}";
    html += ".refresh:hover{background:#334155;}";
    html += "</style></head><body>";
    html += "<div class='card'><h2>VTIC WiFi Config</h2>";
    html += "<form action='/save' method='POST'>";
    html += "<label>WiFi Network</label>";
    html += "<select name='ssid' id='ssid' required onchange=\"document.getElementById('ssid_manual').value=this.value\">" + options + "</select>";
    html += "<label>Or type SSID manually</label>";
    html += "<input id='ssid_manual' name='ssid_manual' placeholder='Type SSID here' oninput=\"if(this.value){document.getElementById('ssid').value=''}\">";
    html += "<label>WiFi Password</label><input name='pass' type='password' placeholder='Leave blank if open'>";
    html += "<label>Firebase Project ID</label><input name='project' value='" + firebaseProject + "' required>";
    html += "<label>Firebase API Key</label><input name='apikey' value='" + firebaseApiKey + "' required>";
    html += "<button type='submit'>Save &amp; Restart</button>";
    html += "</form>";
    html += "<form action='/scan' method='GET'><button type='submit' class='refresh'>&#x21bb; Rescan Networks</button></form>";
    html += "</div></body></html>";
    server.send(200, "text/html", html);
  }

  void handleScan() {
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    // Brief wait so scan has a moment to start, then redirect back
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Scanning...");
  }

  void handleSave() {
    ssid = server.arg("ssid_manual").length() > 0 ? server.arg("ssid_manual") : server.arg("ssid");
    password = server.arg("pass");
    firebaseProject = server.arg("project");
    firebaseApiKey = server.arg("apikey");

    prefs.begin("vtic-voting", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.putString("project", firebaseProject);
    prefs.putString("apikey", firebaseApiKey);
    prefs.end();

    String html = "<html><body><h2>Configuration saved!</h2><p>Terminal is restarting to connect. You can close this window.</p></body></html>";
    server.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
  }

public:
  WifiManager() : server(80), configMode(false) {}

  void begin() {
    prefs.begin("vtic-voting", true);
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("password", "");
    firebaseProject = prefs.getString("project", DEFAULT_FIREBASE_PROJECT_ID);
    firebaseApiKey = prefs.getString("apikey", DEFAULT_FIREBASE_API_KEY);
    prefs.end();
  }

  bool hasCredentials() {
    return ssid.length() > 0;
  }

  String getSSID() { return ssid; }
  String getFirebaseProject() { return firebaseProject; }
  String getFirebaseApiKey() { return firebaseApiKey; }

  void startPortal() {
    configMode = true;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("VTIC-VOTING-SETUP");

    // Scan before starting the AP so networks are ready on first page load
    WiFi.scanNetworks(true);

    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    server.on("/", std::bind(&WifiManager::handleRoot, this));
    server.on("/scan", std::bind(&WifiManager::handleScan, this));
    server.on("/save", std::bind(&WifiManager::handleSave, this));
    server.onNotFound(std::bind(&WifiManager::handleRoot, this));

    server.begin();
  }

  void handlePortal() {
    if (configMode) {
      dnsServer.processNextRequest();
      server.handleClient();
    }
  }

  // Call once after startPortal() — waits for the async scan to finish
  void waitForScan() {
    unsigned long start = millis();
    while (WiFi.scanComplete() == WIFI_SCAN_RUNNING && millis() - start < 10000) {
      delay(100);
    }
  }

  bool connect() {
    if (!hasCredentials()) return false;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 30) {
      delay(500);
      retries++;
    }

    return WiFi.status() == WL_CONNECTED;
  }

  void resetCredentials() {
    prefs.begin("vtic-voting", false);
    prefs.clear();
    prefs.end();
  }
};

#endif
