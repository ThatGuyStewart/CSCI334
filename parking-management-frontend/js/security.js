/* ============================================================
   security.js — Logic for security.html
   ============================================================ */

document.addEventListener('DOMContentLoaded', () => {

  /* ── Inject sidebar (security role) ── */
  initSidebar('security', 'security');

  /* ── Set today's date on verify form ── */
  setTodayDate('verify-date');

  /* ── Render all sections ── */
  renderAlerts();
  renderStatRow();
  renderOccVsPaidTable();
  renderIssuesSummary();
  renderUnpaidTable();
  renderVerifyTable();
  renderActiveBookingsTable();

  /* ── Check bookings button ── */
  document.getElementById('check-bookings-btn').addEventListener('click', () => {
    const area = document.getElementById('verify-area').value;
    showToast(`Bookings loaded for ${area}.`, '✅');
  });
});

/* ── Alert banners at top of overview ── */
function renderAlerts() {
  const container = document.getElementById('security-alerts');
  if (!container) return;

  const alerts = [
    {
      type: 'urgent',
      icon: '🚨',
      title: 'Areas B, C, D — Unpaid Parking Detected',
      body:  'Occupied spaces exceed paid tickets for 14+ minutes. Verification required.',
    },
    {
      type: 'warn',
      icon: '⚠️',
      title: 'Reserved Space Conflict — Area B, 14:15',
      body:  'User with valid booking unable to find a reserved space. Under review.',
    },
    {
      type: 'ok',
      icon: '✅',
      title: 'Area A — All Clear',
      body:  'Occupied spaces match paid ticket count. No issues detected.',
    },
  ];

  container.innerHTML = alerts.map(a => `
    <div class="alert alert-${a.type}">
      <div class="alert-icon">${a.icon}</div>
      <div class="alert-body">
        <div class="alert-title">${a.title}</div>
        ${a.body}
      </div>
    </div>
  `).join('');
}

/* ── Stat row: Occupied / Tickets / Difference ── */
function renderStatRow() {
  const container = document.getElementById('security-stat-row');
  if (!container) return;

  const stats = [
    { label: 'Occupied Spaces', value: '127', colour: '' },
    { label: 'Active Tickets',  value: '119', colour: 'text-green' },
    { label: 'Difference',      value: '8',   colour: 'text-crimson', meta: 'require check' },
  ];

  container.innerHTML = stats.map(s => `
    <div class="stat-box">
      <div class="stat-label">${s.label}</div>
      <div class="stat-value ${s.colour}">${s.value}</div>
      ${s.meta ? `<div class="stat-meta">${s.meta}</div>` : ''}
    </div>
  `).join('');
}

/* ── Occupancy vs Paid table (overview tab) ── */
function renderOccVsPaidTable() {
  const tbody = document.querySelector('#occ-vs-paid-table tbody');
  if (!tbody) return;

  tbody.innerHTML = SECURITY_OCC_VS_PAID.map(row => {
    const diffColour  = row.diff > 0 ? 'text-crimson' : 'text-green';
    const statusStyle = row.status === 'clear'
      ? 'color:var(--green); font-weight:600;'
      : 'color:var(--crimson); font-weight:600;';
    const statusText  = row.status === 'clear' ? 'Clear' : 'Check Required';

    return `
      <tr>
        <td>${row.area}</td>
        <td class="mono">${row.occupied}</td>
        <td class="mono">${row.paid}</td>
        <td class="mono ${diffColour}">${row.diff}</td>
        <td style="${statusStyle}">${statusText}</td>
      </tr>`;
  }).join('');
}

/* ── Issues summary panel (overview tab) ── */
function renderIssuesSummary() {
  const container = document.getElementById('issues-summary');
  if (!container) return;

  container.innerHTML = RESERVED_ISSUES.map((issue, i) => {
    const isLast   = i === RESERVED_ISSUES.length - 1;
    const colour   = issue.status === 'resolved' ? 'var(--green)' : 'var(--amber)';
    const label    = issue.status === 'resolved' ? 'Resolved' : 'Under review';
    const areaCol  = issue.status === 'review' ? 'text-crimson' : '';
    const divider  = isLast ? '' : 'margin-bottom:10px; padding-bottom:10px; border-bottom:1px solid var(--border);';

    return `
      <div style="${divider}">
        <div style="font-size:12px; font-weight:600;" class="${areaCol}">${issue.area}</div>
        <div style="font-size:11px; color:var(--text-3);">${issue.time} | ${issue.type}</div>
        <div style="font-size:11px; color:${colour}; margin-top:2px;">${label}</div>
      </div>`;
  }).join('');
}

/* ── Unpaid parking table (unpaid tab) ── */
function renderUnpaidTable() {
  const tbody = document.querySelector('#unpaid-table tbody');
  if (!tbody) return;

  tbody.innerHTML = UNPAID_PARKING.map(row => {
    const timeCell = row.timeOver
      ? `<td class="mono text-crimson fw-600">${row.timeOver} min</td>`
      : `<td>—</td>`;

    const actionCell = row.timeOver
      ? `<td>
           <button class="btn btn-danger btn-sm"
             onclick="showToast('Dispatching to ${row.area}.', '🚔')">
             Investigate
           </button>
         </td>`
      : `<td style="color:var(--green); font-weight:600;">Clear</td>`;

    return `
      <tr>
        <td>${row.area}</td>
        <td class="mono">${row.occupied}</td>
        <td class="mono">${row.tickets}</td>
        ${timeCell}
        ${actionCell}
      </tr>`;
  }).join('');
}

/* ── Active bookings verification table (verify tab) ── */
function renderVerifyTable() {
  const tbody = document.querySelector('#verify-table tbody');
  if (!tbody) return;

  tbody.innerHTML = ACTIVE_BOOKINGS.map(b => `
    <tr>
      <td class="mono fw-600">${b.reg}</td>
      <td>${b.area}</td>
      <td class="mono">${b.time}</td>
      <td>${statusTag(b.status)}</td>
    </tr>
  `).join('');
}

/* ── All active bookings table (active bookings tab) ── */
function renderActiveBookingsTable() {
  const tbody = document.querySelector('#active-bookings-table tbody');
  if (!tbody) return;

  tbody.innerHTML = ACTIVE_BOOKINGS.map(b => `
    <tr>
      <td class="mono fw-600">${b.reg}</td>
      <td>${b.area}</td>
      <td class="mono">${b.time}</td>
      <td>${statusTag(b.status)}</td>
    </tr>
  `).join('');
}
