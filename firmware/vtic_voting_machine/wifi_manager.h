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
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>VTIC Terminal Setup</title>";
    html += "<style>body{font-family:sans-serif;background:#f1f5f9;margin:0;padding:20px;display:flex;justify-content:center;}";
    html += ".card{background:#fff;padding:24px;border-radius:10px;box-shadow:0 4px 6px rgba(0,0,0,0.05);max-width:400px;width:100%;}";
    html += "h2{margin-top:0;color:#1e293b;}input{width:100%;padding:10px;margin:8px 0 16px;border:1px solid #cbd5e1;border-radius:6px;box-sizing:border-box;}";
    html += "button{width:100%;padding:12px;background:#1d4ed8;color:#fff;border:none;border-radius:6px;font-weight:bold;cursor:pointer;}";
    html += "button:hover{background:#1e40af;}</style></head><body>";
    html += "<div class='card'><h2>VTIC WiFi Config</h2>";
    html += "<form action='/save' method='POST'>";
    html += "<label>WiFi SSID</label><input name='ssid' required placeholder='Enter WLAN SSID'>";
    html += "<label>WiFi Password</label><input name='pass' type='password' placeholder='Leave blank if open'>";
    html += "<label>Firebase Project ID</label><input name='project' value='" + firebaseProject + "' required>";
    html += "<label>Firebase API Key</label><input name='apikey' value='" + firebaseApiKey + "' required>";
    html += "<button type='submit'>Save & Restart</button>";
    html += "</form></div></body></html>";
    server.send(200, "text/html", html);
  }

  void handleSave() {
    ssid = server.arg("ssid");
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
    WiFi.mode(WIFI_AP);
    WiFi.softAP("VTIC-VOTING-SETUP");
    
    // Redirect all requests to local web server
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    server.on("/", std::bind(&WifiManager::handleRoot, this));
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
