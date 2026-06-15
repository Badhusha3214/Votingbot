#ifndef DASHBOARD_SERVER_H
#define DASHBOARD_SERVER_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include "election_config.h"
#include "vote_tracker.h"
#include "config.h"

// ─── Embedded Dashboard HTML ────────────────────────────────────────────────
static const char DASHBOARD_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>VTIC Election Dashboard</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',system-ui,sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh}
a{color:inherit;text-decoration:none}

/* ── Header ── */
.hdr{background:linear-gradient(135deg,#1e3a8a 0%,#1d4ed8 100%);
  padding:14px 24px;display:flex;align-items:center;justify-content:space-between;
  box-shadow:0 4px 24px rgba(0,0,0,.4)}
.hdr-logo{display:flex;align-items:center;gap:10px}
.hdr-logo svg{width:28px;height:28px;fill:#fff}
.hdr-title{font-size:20px;font-weight:800;color:#fff;letter-spacing:.5px}
.hdr-sub{font-size:11px;color:#93c5fd;margin-top:1px}
.hdr-right{display:flex;align-items:center;gap:16px}
.dev-chip{background:rgba(255,255,255,.1);border:1px solid rgba(255,255,255,.2);
  border-radius:20px;padding:4px 12px;font-size:12px;color:#bfdbfe}
.dot{width:9px;height:9px;border-radius:50%;flex-shrink:0}
.dot-on{background:#22c55e;box-shadow:0 0 8px #22c55e80;animation:pulse 2s infinite}
.dot-off{background:#f59e0b}
.dot-err{background:#ef4444}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}
.wifi-info{display:flex;align-items:center;gap:6px;font-size:12px;color:#93c5fd}

/* ── Save Banner ── */
.banner{background:#1e3a5f;border-bottom:2px solid #2563eb;
  padding:10px 24px;display:none;align-items:center;gap:12px;flex-wrap:wrap}
.banner.show{display:flex}
.banner-msg{color:#93c5fd;font-size:13px;flex:1}

/* ── Control Bar ── */
.ctrl{background:#1e293b;border-bottom:1px solid #334155;
  padding:12px 24px;display:flex;align-items:center;gap:10px;flex-wrap:wrap}
.badge{padding:5px 14px;border-radius:20px;font-size:11px;font-weight:700;letter-spacing:1px}
.badge-active{background:#14532d;color:#86efac;border:1px solid #16a34a}
.badge-stopped{background:#7f1d1d;color:#fca5a5;border:1px solid #dc2626}
.sep{width:1px;height:24px;background:#334155;margin:0 4px}
.total{margin-left:auto;font-size:13px;color:#64748b}
.total b{color:#60a5fa;font-size:18px;font-weight:800}
.last-upd{font-size:11px;color:#475569;white-space:nowrap}

/* ── Buttons ── */
.btn{padding:8px 16px;border:none;border-radius:6px;font-size:13px;
  font-weight:600;cursor:pointer;transition:.15s all;display:inline-flex;
  align-items:center;gap:5px;white-space:nowrap}
.btn:active{transform:scale(.96)}
.btn-green{background:#16a34a;color:#fff}
.btn-green:hover{background:#15803d}
.btn-red{background:#dc2626;color:#fff}
.btn-red:hover{background:#b91c1c}
.btn-blue{background:#2563eb;color:#fff}
.btn-blue:hover{background:#1d4ed8}
.btn-gray{background:#475569;color:#fff}
.btn-gray:hover{background:#374151}
.btn-outline{background:transparent;color:#94a3b8;border:1px solid #475569}
.btn-outline:hover{background:#1e293b}
.btn-amber{background:#b45309;color:#fff}
.btn-amber:hover{background:#92400e}

/* ── Grid ── */
.grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;
  padding:24px;max-width:1100px;margin:0 auto}
@media(max-width:720px){.grid{grid-template-columns:1fr;padding:16px}}

/* ── Cards ── */
.card{background:#1e293b;border-radius:12px;overflow:hidden;
  border:1px solid #334155;box-shadow:0 4px 20px rgba(0,0,0,.25);
  transition:.2s border-color}
.card:hover{border-color:#475569}
.card-stripe{height:4px}
.s0 .card-stripe{background:linear-gradient(90deg,#3b82f6,#06b6d4)}
.s1 .card-stripe{background:linear-gradient(90deg,#10b981,#84cc16)}
.s2 .card-stripe{background:linear-gradient(90deg,#f59e0b,#f97316)}
.s3 .card-stripe{background:linear-gradient(90deg,#8b5cf6,#ec4899)}
.card-hdr{padding:14px 18px 10px;display:flex;align-items:flex-start;gap:12px}
.card-num{font-size:10px;color:#64748b;font-weight:700;letter-spacing:1px;
  background:#0f172a;border-radius:4px;padding:2px 7px;margin-top:2px;flex-shrink:0}
.card-name-wrap{flex:1;min-width:0}
.sec-input{background:transparent;border:none;font-size:17px;font-weight:700;
  color:#f1f5f9;width:100%;outline:none;cursor:text;border-bottom:2px solid transparent;
  transition:.2s border-color;padding-bottom:2px}
.sec-input:focus{border-bottom-color:#3b82f6;color:#fff}
.sec-input::placeholder{color:#475569}
.card-total-wrap{text-align:right;flex-shrink:0}
.card-total-num{font-size:26px;font-weight:800;color:#f8fafc;line-height:1}
.card-total-lbl{font-size:10px;color:#64748b;letter-spacing:1px}
.card-body{padding:6px 18px 18px}

/* ── Candidate rows ── */
.cand-row{padding:9px 0;border-top:1px solid #1e3a5f}
.cand-top{display:flex;align-items:center;gap:8px}
.cand-bullet{width:8px;height:8px;border-radius:50%;flex-shrink:0}
.s0 .cand-bullet{background:#3b82f6}
.s1 .cand-bullet{background:#10b981}
.s2 .cand-bullet{background:#f59e0b}
.s3 .cand-bullet{background:#8b5cf6}
.cand-input{background:transparent;border:none;font-size:13px;color:#cbd5e1;
  flex:1;outline:none;cursor:text;min-width:0;border-bottom:1px solid transparent;
  transition:.15s border-color}
.cand-input:focus{border-bottom-color:#3b82f6;color:#f1f5f9}
.cand-count{font-size:20px;font-weight:700;color:#f1f5f9;min-width:30px;text-align:right}
.cand-pct{font-size:11px;color:#64748b;min-width:38px;text-align:right}
.cast-btn{padding:3px 10px;font-size:11px;font-weight:600;
  background:#1d4ed8;border:none;border-radius:4px;color:#fff;
  cursor:pointer;transition:.15s;flex-shrink:0;white-space:nowrap}
.cast-btn:hover{background:#2563eb}
.cast-btn:active{transform:scale(.93)}
.bar-wrap{margin-top:6px;padding-left:16px}
.bar-bg{background:#0f172a;height:6px;border-radius:3px;overflow:hidden;
  border:1px solid #334155}
.bar-fill{height:100%;border-radius:3px;transition:width .5s ease;min-width:3px}
.s0 .bar-fill{background:linear-gradient(90deg,#3b82f6,#06b6d4)}
.s1 .bar-fill{background:linear-gradient(90deg,#10b981,#84cc16)}
.s2 .bar-fill{background:linear-gradient(90deg,#f59e0b,#f97316)}
.s3 .bar-fill{background:linear-gradient(90deg,#8b5cf6,#ec4899)}

/* ── Footer ── */
.footer{text-align:center;padding:20px;color:#334155;font-size:12px}

/* ── Toast ── */
.toast{position:fixed;top:16px;right:16px;background:#1e293b;
  border:1px solid #475569;color:#f1f5f9;padding:12px 20px;
  border-radius:8px;box-shadow:0 8px 24px rgba(0,0,0,.4);font-size:13px;
  z-index:9999;transform:translateY(-80px);transition:.3s transform;pointer-events:none}
.toast.show{transform:translateY(0)}
.toast-ok{border-color:#16a34a}
.toast-err{border-color:#dc2626}

/* ── Spinner ── */
@keyframes spin{to{transform:rotate(360deg)}}
.spin{display:inline-block;width:12px;height:12px;border:2px solid #334155;
  border-top-color:#60a5fa;border-radius:50%;animation:spin .7s linear infinite;vertical-align:middle}
</style>
</head>
<body>

<!-- Header -->
<div class="hdr">
  <div class="hdr-logo">
    <svg viewBox="0 0 24 24"><path d="M9 11l3 3L22 4M21 12v7a2 2 0 01-2 2H5a2 2 0 01-2-2V5a2 2 0 012-2h11"/></svg>
    <div>
      <div class="hdr-title">VTIC Election Dashboard</div>
      <div class="hdr-sub">Smart School Election Terminal</div>
    </div>
  </div>
  <div class="hdr-right">
    <div class="wifi-info">
      <div class="dot" id="wifiDot"></div>
      <span id="wifiLabel">Connecting…</span>
    </div>
    <div class="dev-chip" id="devChip">Device: —</div>
  </div>
</div>

<!-- Unsaved-changes banner -->
<div class="banner" id="banner">
  <span class="banner-msg">You have unsaved changes to the election configuration.</span>
  <button class="btn btn-blue" onclick="saveConfig()">Save Configuration</button>
  <button class="btn btn-outline" onclick="discardChanges()">Discard</button>
</div>

<!-- Control bar -->
<div class="ctrl">
  <span class="badge badge-stopped" id="elBadge">STOPPED</span>
  <button class="btn btn-green" id="btnStart" onclick="startElection()">▶ Start Election</button>
  <button class="btn btn-red"   id="btnStop"  onclick="stopElection()"  style="display:none">⏹ Stop Election</button>
  <div class="sep"></div>
  <button class="btn btn-amber" onclick="resetVotes()">⟳ Reset All Votes</button>
  <button class="btn btn-blue"  onclick="saveConfig()">✓ Save Config</button>
  <div class="total">Total Votes Cast: <b id="totalVotes">0</b></div>
  <div class="last-upd"><span class="spin" id="spinner" style="display:none"></span> <span id="lastUpd">—</span></div>
</div>

<!-- 4-section grid -->
<div class="grid" id="grid"></div>

<!-- Footer -->
<div class="footer">VTIC Voting Machine • Web Dashboard • Auto-refreshes every 3 s</div>

<!-- Toast notification -->
<div class="toast" id="toast"></div>

<script>
// ── State ──────────────────────────────────────────────────────────────────
let cfg = {
  active: false,
  sections: [
    {name:"Head Boy",      candidates:["Candidate 1","Candidate 2","Candidate 3"]},
    {name:"Head Girl",     candidates:["Candidate 1","Candidate 2","Candidate 3"]},
    {name:"House Leader",  candidates:["Candidate 1","Candidate 2","Candidate 3"]},
    {name:"Sports Captain",candidates:["Candidate 1","Candidate 2","Candidate 3"]}
  ]
};
let votes = [[0,0,0],[0,0,0],[0,0,0],[0,0,0]];
let dirty = false;
let polling = null;

// ── Utilities ──────────────────────────────────────────────────────────────
function esc(s){
  return String(s).replace(/[<>"'&]/g,c=>({'<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;','&':'&amp;'}[c]));
}
function toast(msg, ok=true){
  const t=document.getElementById('toast');
  t.textContent=msg;
  t.className='toast show '+(ok?'toast-ok':'toast-err');
  clearTimeout(t._tid);
  t._tid=setTimeout(()=>t.classList.remove('show'),3200);
}
function markDirty(){
  dirty=true;
  document.getElementById('banner').classList.add('show');
}
function discardChanges(){
  dirty=false;
  document.getElementById('banner').classList.remove('show');
  fetchConfig();
}

// ── Render ─────────────────────────────────────────────────────────────────
function renderGrid(){
  const grid=document.getElementById('grid');
  const active=cfg.active;
  let html='';
  for(let s=0;s<4;s++){
    const sec=cfg.sections[s]||{name:'Section '+(s+1),candidates:['Candidate 1','Candidate 2','Candidate 3']};
    const sv=votes[s]||[0,0,0];
    const total=sv.reduce((a,b)=>a+b,0);

    let rows='';
    for(let c=0;c<3;c++){
      const v=sv[c]||0;
      const pct=total>0?Math.round(v/total*100):0;
      const barW=total>0?Math.max(pct,2):2;
      rows+=`<div class="cand-row">
        <div class="cand-top">
          <div class="cand-bullet"></div>
          <input class="cand-input" value="${esc(sec.candidates[c])}"
            data-s="${s}" data-c="${c}"
            onchange="cfg.sections[${s}].candidates[${c}]=this.value;markDirty()"
            placeholder="Candidate ${c+1}">
          <div class="cand-count">${v}</div>
          <div class="cand-pct">${pct}%</div>
          ${active?`<button class="cast-btn" onclick="castVote(${s},${c})">+Vote</button>`:''}
        </div>
        <div class="bar-wrap">
          <div class="bar-bg"><div class="bar-fill" style="width:${barW}%"></div></div>
        </div>
      </div>`;
    }

    html+=`<div class="card s${s}">
      <div class="card-stripe"></div>
      <div class="card-hdr">
        <div class="card-num">SECTION ${s+1}</div>
        <div class="card-name-wrap">
          <input class="sec-input" value="${esc(sec.name)}"
            onchange="cfg.sections[${s}].name=this.value;markDirty()"
            placeholder="Election Name">
        </div>
        <div class="card-total-wrap">
          <div class="card-total-num">${total}</div>
          <div class="card-total-lbl">VOTES</div>
        </div>
      </div>
      <div class="card-body">${rows}</div>
    </div>`;
  }
  grid.innerHTML=html;
}

// ── Data fetch helpers ─────────────────────────────────────────────────────
async function fetchVotes(){
  const r=await fetch('/api/votes');
  if(!r.ok) throw new Error('votes '+r.status);
  votes=await r.json();
}
async function fetchConfig(){
  const r=await fetch('/api/config');
  if(!r.ok) throw new Error('config '+r.status);
  cfg=await r.json();
}
async function fetchStatus(){
  const r=await fetch('/api/status');
  if(!r.ok) throw new Error('status '+r.status);
  return r.json();
}

// ── Master refresh ─────────────────────────────────────────────────────────
async function refresh(){
  document.getElementById('spinner').style.display='inline-block';
  try{
    const [,st]=await Promise.all([fetchVotes(), fetchStatus()]);
    if(!dirty) await fetchConfig();

    // Status bar
    const dot=document.getElementById('wifiDot');
    const lbl=document.getElementById('wifiLabel');
    if(st.wifi){
      dot.className='dot dot-on';
      lbl.textContent='Online — '+st.ssid;
    } else {
      dot.className='dot dot-off';
      lbl.textContent='Offline mode';
    }
    document.getElementById('devChip').textContent='Device: '+st.deviceId;

    // Election badge
    const badge=document.getElementById('elBadge');
    const isActive=cfg.active;
    badge.textContent=isActive?'ACTIVE':'STOPPED';
    badge.className='badge '+(isActive?'badge-active':'badge-stopped');
    document.getElementById('btnStart').style.display=isActive?'none':'';
    document.getElementById('btnStop').style.display =isActive?'':'none';

    // Total
    const total=votes.flat().reduce((a,b)=>a+b,0);
    document.getElementById('totalVotes').textContent=total;
    document.getElementById('lastUpd').textContent='Updated '+new Date().toLocaleTimeString();

    renderGrid();
  }catch(e){
    const dot=document.getElementById('wifiDot');
    dot.className='dot dot-err';
    document.getElementById('wifiLabel').textContent='Connection error';
  }finally{
    document.getElementById('spinner').style.display='none';
  }
}

// ── Actions ────────────────────────────────────────────────────────────────
async function saveConfig(){
  try{
    const r=await fetch('/api/config',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify(cfg)
    });
    if(r.ok){
      dirty=false;
      document.getElementById('banner').classList.remove('show');
      toast('Configuration saved!');
    } else toast('Failed to save: '+r.status, false);
  }catch(e){toast('Network error',false);}
}

async function resetVotes(){
  if(!confirm('Reset ALL vote counts to zero? This cannot be undone.')) return;
  try{
    const r=await fetch('/api/reset-votes',{method:'POST'});
    if(r.ok){ toast('All votes reset to zero.'); refresh(); }
    else toast('Reset failed',false);
  }catch(e){toast('Network error',false);}
}

async function startElection(){
  try{
    const r=await fetch('/api/election/start',{method:'POST'});
    if(r.ok){ toast('Election is now ACTIVE!'); if(!dirty) cfg.active=true; refresh(); }
    else toast('Failed to start election',false);
  }catch(e){toast('Network error',false);}
}

async function stopElection(){
  if(!confirm('Stop the election? Physical buttons will be disabled.')) return;
  try{
    const r=await fetch('/api/election/stop',{method:'POST'});
    if(r.ok){ toast('Election stopped.'); if(!dirty) cfg.active=false; refresh(); }
    else toast('Failed to stop election',false);
  }catch(e){toast('Network error',false);}
}

async function castVote(section,candidate){
  try{
    const r=await fetch('/api/vote',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({section,candidate})
    });
    if(r.ok){
      votes[section][candidate]++;
      renderGrid();
    } else {
      const txt=await r.text();
      toast(txt||'Vote failed',false);
    }
  }catch(e){toast('Network error',false);}
}

// ── Boot ──────────────────────────────────────────────────────────────────
refresh();
polling=setInterval(refresh,3000);
</script>
</body>
</html>
)rawhtml";
// ─── End embedded HTML ──────────────────────────────────────────────────────

class DashboardServer {
private:
  WebServer       server;
  ElectionConfig* elCfg;
  VoteTracker*    tracker;
  String          deviceId;
  String          ssid;

  void sendJson(int code, const String& body) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(code, "application/json", body);
  }

  void handleRoot() {
    server.sendHeader("Content-Encoding", "identity");
    server.send_P(200, "text/html", DASHBOARD_HTML);
  }

  void handleApiVotes() {
    sendJson(200, tracker->toJson());
  }

  void handleApiConfig() {
    sendJson(200, elCfg->toJson());
  }

  void handleApiSaveConfig() {
    if (!server.hasArg("plain")) { sendJson(400, "{\"error\":\"No body\"}"); return; }
    String body = server.arg("plain");
    if (!elCfg->fromJson(body)) { sendJson(400, "{\"error\":\"Invalid JSON\"}"); return; }
    elCfg->save();
    sendJson(200, "{\"ok\":true}");
  }

  void handleApiStatus() {
    StaticJsonDocument<256> doc;
    doc["deviceId"]     = deviceId;
    doc["wifi"]         = (WiFi.status() == WL_CONNECTED);
    doc["ssid"]         = ssid;
    doc["ip"]           = WiFi.localIP().toString();
    doc["totalVotes"]   = tracker->getTotal();
    String out;
    serializeJson(doc, out);
    sendJson(200, out);
  }

  void handleApiResetVotes() {
    tracker->reset();
    sendJson(200, "{\"ok\":true}");
  }

  void handleApiStartElection() {
    elCfg->active = true;
    elCfg->save();
    sendJson(200, "{\"ok\":true}");
  }

  void handleApiStopElection() {
    elCfg->active = false;
    elCfg->save();
    sendJson(200, "{\"ok\":true}");
  }

  void handleApiCastVote() {
    if (!elCfg->active) { server.send(403, "text/plain", "Election is not active"); return; }
    if (!server.hasArg("plain")) { sendJson(400, "{\"error\":\"No body\"}"); return; }
    StaticJsonDocument<64> doc;
    if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
      sendJson(400, "{\"error\":\"Bad JSON\"}"); return;
    }
    int section   = doc["section"]   | -1;
    int candidate = doc["candidate"] | -1;
    if (!tracker->castVote(section, candidate)) {
      sendJson(400, "{\"error\":\"Invalid section or candidate\"}"); return;
    }
    sendJson(200, "{\"ok\":true}");
  }

  void handleNotFound() {
    server.send(404, "text/plain", "Not found");
  }

public:
  DashboardServer() : server(80) {}

  void begin(ElectionConfig* cfg, VoteTracker* trk,
             const String& devId, const String& wifiSsid) {
    elCfg    = cfg;
    tracker  = trk;
    deviceId = devId;
    ssid     = wifiSsid;

    server.on("/",                    [this]{ handleRoot(); });
    server.on("/api/votes",           [this]{ handleApiVotes(); });
    server.on("/api/config",  HTTP_GET,  [this]{ handleApiConfig(); });
    server.on("/api/config",  HTTP_POST, [this]{ handleApiSaveConfig(); });
    server.on("/api/status",           [this]{ handleApiStatus(); });
    server.on("/api/reset-votes", HTTP_POST, [this]{ handleApiResetVotes(); });
    server.on("/api/election/start",  HTTP_POST, [this]{ handleApiStartElection(); });
    server.on("/api/election/stop",   HTTP_POST, [this]{ handleApiStopElection(); });
    server.on("/api/vote",    HTTP_POST, [this]{ handleApiCastVote(); });
    server.onNotFound([this]{ handleNotFound(); });

    server.begin();
    Serial.println("Dashboard server started on port 80.");
  }

  void loop() {
    server.handleClient();
  }

  void updateSsid(const String& s) { ssid = s; }
};

#endif
