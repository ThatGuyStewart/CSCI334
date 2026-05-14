/* ============================================================
   admin.js — Logic for admin.html
   ============================================================ */

document.addEventListener('DOMContentLoaded', () => {

  /* ── Inject sidebar (admin role) ── */
  initSidebar('admin', 'admin');

  /* ── Render all sections ── */
  renderStatRow();
  renderSummaryTable();
  renderPeakOverview();
  renderWeeklyChart();
  renderUtilBars();
  renderAllPeakBars();
  renderUtilTable();
});

/* ── Top stat row ── */
function renderStatRow() {
  const container = document.getElementById('admin-stat-row');
  if (!container) return;

  const stats = [
    { label: 'Total Spaces',    value: '1,200', colour: '' },
    { label: 'Occupied Spaces', value: '847',   colour: 'text-crimson' },
    { label: 'Available Spaces',value: '353',   colour: 'text-green' },
    { label: 'Occupancy Rate',  value: '70.6%', colour: '' },
  ];

  container.innerHTML = stats.map(s => `
    <div class="stat-box">
      <div class="stat-label">${s.label}</div>
      <div class="stat-value ${s.colour}">${s.value}</div>
    </div>
  `).join('');
}

/* ── Area summary table (overview tab) ── */
function renderSummaryTable() {
  const tbody = document.querySelector('#admin-summary-table tbody');
  if (!tbody) return;

  tbody.innerHTML = ADMIN_AREA_SUMMARY.map(row => `
    <tr>
      <td>${row.area}</td>
      <td class="mono">${row.occupied}</td>
      <td class="mono">${row.available}</td>
      <td>${statusTag(row.status)}</td>
    </tr>
  `).join('');
}

/* ── Peak hours (overview tab — first 3 only) ── */
function renderPeakOverview() {
  const container = document.getElementById('admin-peak-overview');
  if (!container) return;

  container.innerHTML = PEAK_HOURS.slice(0, 3).map(row => `
    <div class="peak-row">
      <div class="peak-time">${row.time}</div>
      <div class="peak-bar-wrap">
        <div class="peak-fill" style="width:${row.pct}%"></div>
      </div>
      <div class="peak-pct">${row.pct}%</div>
    </div>
  `).join('');
}

/* ── Weekly bar chart (stats tab) ── */
function renderWeeklyChart() {
  const container = document.getElementById('weekly-bar-chart');
  if (!container) return;

  container.innerHTML = WEEKLY_USAGE.map(d => `
    <div class="bar-col">
      <div class="bar" style="height:${d.height}px; opacity:${d.opacity};"></div>
      <div class="bar-day">${d.day}</div>
    </div>
  `).join('');
}

/* ── Utilisation progress bars (stats tab) ── */
function renderUtilBars() {
  const container = document.getElementById('util-bars-stats');
  if (!container) return;

  container.innerHTML = UTILISATION.map(row => `
    <div class="util-row">
      <div class="util-header">
        <span>${row.area}</span>
        <span class="util-pct">${row.pct}%</span>
      </div>
      <div class="util-bar">
        <div class="util-fill ${row.level}" style="width:${row.pct}%"></div>
      </div>
    </div>
  `).join('');
}

/* ── All peak hours bars (peak tab) ── */
function renderAllPeakBars() {
  const container = document.getElementById('all-peak-bars');
  if (!container) return;

  container.innerHTML = PEAK_HOURS.map(row => `
    <div class="peak-row">
      <div class="peak-time">${row.time}</div>
      <div class="peak-bar-wrap">
        <div class="peak-fill" style="width:${row.pct}%"></div>
      </div>
      <div class="peak-pct">${row.pct}%</div>
    </div>
  `).join('');
}

/* ── Utilisation table (util tab) ── */
function renderUtilTable() {
  const tbody = document.querySelector('#util-table tbody');
  if (!tbody) return;

  const tableData = [
    { area: 'Area A – Main Campus',    total: 200, avg: 181, peak: 200, pct: 90, status: 'high'   },
    { area: 'Area B – Innovation',     total: 200, avg: 146, peak: 180, pct: 73, status: 'medium' },
    { area: 'Area C – Sports Complex', total: 250, avg: 220, peak: 248, pct: 88, status: 'high'   },
    { area: 'Area D – Library',        total: 200, avg: 98,  peak: 145, pct: 49, status: 'low'    },
    { area: 'Area E – Residences',     total: 350, avg: 203, peak: 310, pct: 58, status: 'medium' },
  ];

  tbody.innerHTML = tableData.map(row => `
    <tr>
      <td><div class="tbl-area">${row.area}</div></td>
      <td class="mono">${row.total}</td>
      <td class="mono">${row.avg}</td>
      <td class="mono">${row.peak}</td>
      <td class="mono">${row.pct}%</td>
      <td>${statusTag(row.status)}</td>
    </tr>
  `).join('');
}
