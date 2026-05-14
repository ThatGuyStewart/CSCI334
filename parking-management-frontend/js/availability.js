/* ============================================================
   availability.js — Logic for availability.html
   ============================================================ */

document.addEventListener('DOMContentLoaded', () => {

  /* ── Inject sidebar ── */
  initSidebar('availability', 'user');

  /* ── Start clock ── */
  startClock('avail-clock');

  /* ── Set today's date ── */
  setTodayDate('av-date');

  /* ── Populate area dropdown filter ── */
  populateAreaFilter();

  /* ── Render initial data ── */
  renderPredicted();
  renderAreaTable();
  renderPrices();

  /* ── Check availability button ── */
  document.getElementById('check-avail-btn').addEventListener('click', () => {
    showToast('Availability updated.', '◎');
    renderAreaTable(); // re-render with same data (would call API in real app)
  });
});

/* ── Populate the area dropdown from PARKING_AREAS data ── */
function populateAreaFilter() {
  const select = document.getElementById('av-area');
  if (!select) return;

  PARKING_AREAS.forEach(area => {
    const opt = document.createElement('option');
    opt.value = area.id;
    opt.textContent = `${area.name} – ${area.location}`;
    select.appendChild(opt);
  });
}

/* ── Render predicted availability rows ── */
function renderPredicted() {
  const container = document.getElementById('predicted-list');
  if (!container) return;

  const predictions = [
    { area: 'Area A – Main Campus',       time: 'Tomorrow, 9:00 AM', count: '~45', colour: 'text-green' },
    { area: 'Area B – Innovation Campus', time: 'Tomorrow, 9:00 AM', count: '~28', colour: 'text-amber' },
    { area: 'Area C – Sports Complex',    time: 'Tomorrow, 9:00 AM', count: '~62', colour: 'text-green' },
  ];

  container.innerHTML = predictions.map(p => `
    <div class="pred-row">
      <div>
        <div class="pred-area">${p.area}</div>
        <div class="pred-time">${p.time}</div>
      </div>
      <div style="text-align:right;">
        <div class="pred-count ${p.colour}">${p.count}</div>
        <div class="pred-label">spaces predicted</div>
      </div>
    </div>
  `).join('');
}

/* ── Render the parking areas table ── */
function renderAreaTable() {
  const tbody = document.querySelector('#avail-areas-table tbody');
  if (!tbody) return;

  tbody.innerHTML = PARKING_AREAS.slice(0, 5).map(area => `
    <tr>
      <td>
        <div class="tbl-area">${area.name}</div>
        <div class="tbl-sub">${area.location}</div>
      </td>
      <td class="tbl-count ${countColour(area.status)}">${area.available}</td>
      <td>${statusTag(area.status)}</td>
    </tr>
  `).join('');
}

/* ── Render prices ── */
function renderPrices() {
  const container = document.getElementById('avail-prices');
  if (!container) return;

  const prices = [
    { label: 'Hourly Rate', amount: '$4.00' },
    { label: 'Daily Rate',  amount: '$15.00' },
    { label: 'Other Rates', amount: 'Vary' },
  ];

  container.innerHTML = prices.map(p => `
    <div class="price-row">
      <span>${p.label}</span>
      <span class="price-amount">${p.amount}</span>
    </div>
  `).join('');
}
