'use client';

import { useState, useEffect, useCallback } from 'react';
import { db } from '../lib/firebase';
import {
  doc, collection,
  onSnapshot, setDoc, addDoc,
  getDocs, writeBatch, query, orderBy,
} from 'firebase/firestore';

// ── Defaults written to Firestore on first load ──────────────────────────────
const DEFAULTS = {
  active: false,
  sections: [
    { name: 'Head Boy',       candidates: ['Candidate 1', 'Candidate 2', 'Candidate 3'] },
    { name: 'Head Girl',      candidates: ['Candidate 1', 'Candidate 2', 'Candidate 3'] },
    { name: 'House Leader',   candidates: ['Candidate 1', 'Candidate 2', 'Candidate 3'] },
    { name: 'Sports Captain', candidates: ['Candidate 1', 'Candidate 2', 'Candidate 3'] },
  ],
};

const STRIPE_COLORS = [
  ['#3b82f6', '#06b6d4'],
  ['#10b981', '#84cc16'],
  ['#f59e0b', '#f97316'],
  ['#8b5cf6', '#ec4899'],
];

function clone(obj) { return JSON.parse(JSON.stringify(obj)); }

// ── Vote-count aggregation from raw Firestore docs ───────────────────────────
function aggregateVotes(docs) {
  const counts = Array.from({ length: 4 }, () => [0, 0, 0]);
  docs.forEach(d => {
    const data = d.data();
    for (let s = 0; s < 4; s++) {
      const c = data[`section${s}`];
      if (typeof c === 'number' && c >= 0 && c < 3) counts[s][c]++;
    }
  });
  return counts;
}

// ── Sub-components ────────────────────────────────────────────────────────────

function SectionCard({ sectionIndex, section, voteCounts, onSectionName, onCandidateName }) {
  const total = voteCounts.reduce((a, b) => a + b, 0);
  const cls   = `card s${sectionIndex}`;

  return (
    <div className={cls}>
      <div className="card-stripe" />
      <div className="card-hdr">
        <div className="card-num">SECTION {sectionIndex + 1}</div>
        <div className="card-name-wrap">
          <input
            className="sec-input"
            value={section.name}
            onChange={e => onSectionName(sectionIndex, e.target.value)}
            placeholder="Election name"
          />
        </div>
        <div className="card-total">
          <div className="card-total-num">{total}</div>
          <div className="card-total-lbl">VOTES</div>
        </div>
      </div>

      <div className="card-body">
        {section.candidates.map((name, c) => {
          const v   = voteCounts[c] ?? 0;
          const pct = total > 0 ? Math.round((v / total) * 100) : 0;
          return (
            <div className="cand-row" key={c}>
              <div className="cand-top">
                <div className="cand-bullet" />
                <input
                  className="cand-input"
                  value={name}
                  onChange={e => onCandidateName(sectionIndex, c, e.target.value)}
                  placeholder={`Candidate ${c + 1}`}
                />
                <div className="cand-count">{v}</div>
                <div className="cand-pct">{pct}%</div>
              </div>
              <div className="bar-wrap">
                <div className="bar-bg">
                  <div className="bar-fill" style={{ width: `${total > 0 ? Math.max(pct, 1) : 0}%` }} />
                </div>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}

function DevicesPanel({ devices }) {
  if (devices.length === 0) return null;
  return (
    <div className="devices">
      <div className="devices-title">CONNECTED TERMINALS</div>
      {devices.map(dev => {
        const isOnline = dev.online && (Date.now() / 1000 - (dev.lastActive ?? 0)) < 120;
        const pending  = dev.pendingVotes ?? 0;
        const lastSeen = dev.lastActive
          ? new Date(dev.lastActive * 1000).toLocaleTimeString()
          : '—';
        return (
          <div className="device-row" key={dev.id}>
            <div className="dot" style={{ background: isOnline ? '#22c55e' : '#94a3b8', boxShadow: isOnline ? '0 0 8px #22c55e80' : 'none' }} />
            <div className="device-id">{dev.id}</div>
            <div className="device-meta">
              <span>📶 {dev.ssid || '—'}</span>
              <span>🌐 {dev.ipAddress || '—'}</span>
              <span>🕒 {lastSeen}</span>
            </div>
            {pending > 0 && <div className="device-pending">⚠ {pending} pending</div>}
          </div>
        );
      })}
    </div>
  );
}

// ── Election History ──────────────────────────────────────────────────────────

function HistorySection({ section, sectionIndex }) {
  const total = section.votes.reduce((a, b) => a + b, 0);
  const winner = section.votes.indexOf(Math.max(...section.votes));
  const [from, to] = STRIPE_COLORS[sectionIndex] ?? ['#64748b', '#94a3b8'];

  return (
    <div className="hist-section">
      <div className="hist-section-stripe" style={{ background: `linear-gradient(90deg, ${from}, ${to})` }} />
      <div className="hist-section-hdr">
        <span className="hist-section-name">{section.name}</span>
        <span className="hist-section-total">{total} votes</span>
      </div>
      {section.candidates.map((name, c) => {
        const v   = section.votes[c] ?? 0;
        const pct = total > 0 ? Math.round((v / total) * 100) : 0;
        return (
          <div className="hist-cand" key={c}>
            <div className="hist-cand-top">
              {c === winner && total > 0 && <span className="hist-winner-badge">🏆</span>}
              <span className={`hist-cand-name ${c === winner && total > 0 ? 'hist-winner' : ''}`}>{name}</span>
              <span className="hist-cand-votes">{v}</span>
              <span className="hist-cand-pct">{pct}%</span>
            </div>
            <div className="hist-bar-bg">
              <div
                className="hist-bar-fill"
                style={{
                  width: `${total > 0 ? Math.max(pct, 1) : 0}%`,
                  background: `linear-gradient(90deg, ${from}, ${to})`,
                }}
              />
            </div>
          </div>
        );
      })}
    </div>
  );
}

function HistoryCard({ record, defaultOpen }) {
  const [open, setOpen] = useState(defaultOpen ?? false);
  const date = new Date(record.archivedAt);
  const dateStr = date.toLocaleDateString(undefined, { day: 'numeric', month: 'short', year: 'numeric' });
  const timeStr = date.toLocaleTimeString(undefined, { hour: '2-digit', minute: '2-digit' });

  return (
    <div className="hist-card">
      <button className="hist-card-hdr" onClick={() => setOpen(o => !o)}>
        <div className="hist-card-left">
          <span className="hist-card-icon">🗳️</span>
          <div>
            <div className="hist-card-date">{dateStr} · {timeStr}</div>
            <div className="hist-card-meta">{record.totalVotes} total votes · {record.sections?.length ?? 0} sections</div>
          </div>
        </div>
        <span className="hist-chevron">{open ? '▲' : '▼'}</span>
      </button>

      {open && (
        <div className="hist-card-body">
          <div className="hist-sections-grid">
            {(record.sections ?? []).map((sec, i) => (
              <HistorySection key={i} section={sec} sectionIndex={i} />
            ))}
          </div>
        </div>
      )}
    </div>
  );
}

function ElectionHistory() {
  const [records, setRecords]  = useState(null);
  const [error,   setError]    = useState(null);

  useEffect(() => {
    const q = query(collection(db, 'elections'), orderBy('archivedAt', 'desc'));
    const unsub = onSnapshot(
      q,
      snap => setRecords(snap.docs.map(d => ({ id: d.id, ...d.data() }))),
      err  => setError(err.message),
    );
    return unsub;
  }, []);

  if (error) return (
    <div className="hist-empty">
      <p style={{ color: 'var(--red)' }}>Failed to load history: {error}</p>
    </div>
  );

  if (records === null) return (
    <div className="loading"><div className="spinner" /></div>
  );

  if (records.length === 0) return (
    <div className="hist-empty">
      <div className="hist-empty-icon">📭</div>
      <div className="hist-empty-title">No past elections yet</div>
      <div className="hist-empty-sub">
        When you reset votes, the current results are automatically archived here.
      </div>
    </div>
  );

  return (
    <div className="hist-list">
      <div className="hist-list-hdr">
        <span className="hist-list-title">PAST ELECTIONS</span>
        <span className="hist-list-count">{records.length} record{records.length !== 1 ? 's' : ''}</span>
      </div>
      {records.map((r, i) => (
        <HistoryCard key={r.id} record={r} defaultOpen={i === 0} />
      ))}
    </div>
  );
}

// ── Main Dashboard ────────────────────────────────────────────────────────────
export default function Dashboard() {
  const [cfg,     setCfg]     = useState(null);
  const [votes,   setVotes]   = useState([[0,0,0],[0,0,0],[0,0,0],[0,0,0]]);
  const [devices, setDevices] = useState([]);
  const [fbError, setFbError] = useState(null);
  const [tab,     setTab]     = useState('live');   // 'live' | 'history'

  const [draft,  setDraft]  = useState(null);
  const [dirty,  setDirty]  = useState(false);
  const [saving, setSaving] = useState(false);
  const [toast,  setToast]  = useState(null);

  const display = draft ?? cfg ?? DEFAULTS;

  // ── Firebase listeners ──────────────────────────────────────────────────
  useEffect(() => {
    const cfgUnsub = onSnapshot(
      doc(db, 'config', 'election'),
      snap => {
        if (snap.exists()) {
          setCfg(snap.data());
        } else {
          setDoc(doc(db, 'config', 'election'), DEFAULTS).catch(() => {});
          setCfg(DEFAULTS);
        }
      },
      err => setFbError(err.message),
    );

    const votesUnsub = onSnapshot(
      collection(db, 'votes'),
      snap => setVotes(aggregateVotes(snap.docs)),
      err  => console.error('Votes listener:', err),
    );

    const devsUnsub = onSnapshot(
      collection(db, 'devices'),
      snap => setDevices(snap.docs.map(d => ({ id: d.id, ...d.data() }))),
      err  => console.error('Devices listener:', err),
    );

    return () => { cfgUnsub(); votesUnsub(); devsUnsub(); };
  }, []);

  // ── Toast helper ────────────────────────────────────────────────────────
  const showToast = useCallback((msg, ok = true) => {
    setToast({ msg, ok });
    setTimeout(() => setToast(null), 3200);
  }, []);

  // ── Editing callbacks ───────────────────────────────────────────────────
  function updateSectionName(s, name) {
    setDraft(prev => {
      const d = clone(prev ?? cfg ?? DEFAULTS);
      d.sections[s].name = name;
      return d;
    });
    setDirty(true);
  }

  function updateCandidateName(s, c, name) {
    setDraft(prev => {
      const d = clone(prev ?? cfg ?? DEFAULTS);
      d.sections[s].candidates[c] = name;
      return d;
    });
    setDirty(true);
  }

  function discardChanges() {
    setDraft(null);
    setDirty(false);
  }

  // ── Actions ─────────────────────────────────────────────────────────────
  async function saveConfig() {
    if (!dirty || !draft) return;
    setSaving(true);
    try {
      await setDoc(doc(db, 'config', 'election'), draft);
      setDraft(null);
      setDirty(false);
      showToast('Configuration saved!');
    } catch (e) {
      showToast('Save failed: ' + e.message, false);
    }
    setSaving(false);
  }

  async function setActive(active) {
    const base = clone(draft ?? cfg ?? DEFAULTS);
    base.active = active;
    try {
      await setDoc(doc(db, 'config', 'election'), base);
      if (draft) setDraft(d => ({ ...d, active }));
      showToast(active ? '✅ Election is now ACTIVE!' : '🛑 Election stopped.');
    } catch (e) {
      showToast('Failed: ' + e.message, false);
    }
  }

  async function resetVotes() {
    const totalVotes = votes.flat().reduce((a, b) => a + b, 0);

    const confirmMsg = totalVotes > 0
      ? `Archive and reset ALL ${totalVotes} votes?\n\nThis will save results to History then clear the current election.`
      : 'No votes recorded. Clear the election anyway?';

    if (!confirm(confirmMsg)) return;

    try {
      // 1. Archive current results before deleting
      if (totalVotes > 0) {
        const currentCfg = cfg ?? DEFAULTS;
        const archiveData = {
          archivedAt: new Date().toISOString(),
          totalVotes,
          sections: currentCfg.sections.map((sec, s) => ({
            name: sec.name,
            candidates: sec.candidates,
            votes: votes[s] ?? [0, 0, 0],
          })),
        };
        await addDoc(collection(db, 'elections'), archiveData);
      }

      // 2. Batch-delete all vote documents
      const snap = await getDocs(collection(db, 'votes'));
      if (snap.size > 0) {
        let remaining = [...snap.docs];
        while (remaining.length > 0) {
          const batch = writeBatch(db);
          remaining.splice(0, 400).forEach(d => batch.delete(d.ref));
          await batch.commit();
        }
      }

      showToast(totalVotes > 0 ? '✅ Results archived & votes reset.' : 'Votes cleared.');
    } catch (e) {
      showToast('Reset failed: ' + e.message, false);
    }
  }

  // ── Derived ─────────────────────────────────────────────────────────────
  const totalVotes = votes.flat().reduce((a, b) => a + b, 0);
  const isActive   = display.active;

  // ── Error screen ────────────────────────────────────────────────────────
  if (fbError) {
    return (
      <div className="error-screen">
        <h2>Firebase connection error</h2>
        <p>Check that your environment variables are set correctly.</p>
        <code>{fbError}</code>
      </div>
    );
  }

  if (!cfg) {
    return <div className="loading"><div className="spinner" /></div>;
  }

  // ── Render ───────────────────────────────────────────────────────────────
  return (
    <>
      {/* Header */}
      <header className="hdr">
        <div className="hdr-logo">
          <span className="hdr-icon">🗳️</span>
          <div>
            <div className="hdr-title">VTIC Election Dashboard</div>
            <div className="hdr-sub">Smart School Election Terminal</div>
          </div>
        </div>
        <div className="hdr-center">
          <button className={`tab-btn ${tab === 'live' ? 'tab-active' : ''}`} onClick={() => setTab('live')}>
            Live Results
          </button>
          <button className={`tab-btn ${tab === 'history' ? 'tab-active' : ''}`} onClick={() => setTab('history')}>
            History
          </button>
        </div>
        <div className="hdr-right">
          <div className="wifi-row">
            <div className={`dot ${devices.length > 0 ? 'dot-on' : 'dot-off'}`} />
            <span>{devices.length > 0 ? `${devices.length} terminal${devices.length > 1 ? 's' : ''} online` : 'No terminals'}</span>
          </div>
        </div>
      </header>

      {/* ── LIVE TAB ── */}
      {tab === 'live' && (
        <>
          {dirty && (
            <div className="banner">
              <span className="banner-msg">You have unsaved changes to the election configuration.</span>
              <button className="btn btn-blue" onClick={saveConfig} disabled={saving}>
                {saving ? 'Saving…' : '✓ Save Configuration'}
              </button>
              <button className="btn btn-outline" onClick={discardChanges}>Discard</button>
            </div>
          )}

          <div className="ctrl">
            <span className={`badge ${isActive ? 'badge-active' : 'badge-stopped'}`}>
              {isActive ? 'ACTIVE' : 'STOPPED'}
            </span>

            {!isActive
              ? <button className="btn btn-green" onClick={() => setActive(true)}>▶ Start Election</button>
              : <button className="btn btn-red"   onClick={() => setActive(false)}>⏹ Stop Election</button>
            }

            <div className="ctrl-sep" />

            <button className="btn btn-amber" onClick={resetVotes}>⟳ Archive &amp; Reset Votes</button>
            <button className="btn btn-blue"  onClick={saveConfig} disabled={!dirty || saving}>
              {saving ? 'Saving…' : '✓ Save Config'}
            </button>

            <div className="ctrl-right">
              <span className="total-label">Total Votes: <span className="total-num">{totalVotes}</span></span>
            </div>
          </div>

          <div className="grid">
            {display.sections.map((section, s) => (
              <SectionCard
                key={s}
                sectionIndex={s}
                section={section}
                voteCounts={votes[s] ?? [0, 0, 0]}
                active={isActive}
                onSectionName={updateSectionName}
                onCandidateName={updateCandidateName}
              />
            ))}
          </div>

          <DevicesPanel devices={devices} />
        </>
      )}

      {/* ── HISTORY TAB ── */}
      {tab === 'history' && <ElectionHistory />}

      <footer className="footer">
        VTIC Voting Machine · Dashboard on Vercel · Data live from Firebase Firestore
      </footer>

      {toast && (
        <div className={`toast ${toast.ok ? 'toast-ok' : 'toast-err'}`}>
          {toast.msg}
        </div>
      )}
    </>
  );
}
