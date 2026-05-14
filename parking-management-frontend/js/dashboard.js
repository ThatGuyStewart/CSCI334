/* ============================================================
   dashboard.js — Logic for dashboard.html
   ============================================================ */

document.addEventListener('DOMContentLoaded', () => {

  /* ── Inject sidebar ── */
  initSidebar('dashboard', 'user');

  /* ── Start live clock ── */
  startClock('dash-clock');

  /* ── Set today's date on the predicted availability form ── */
  setTodayDate('dash-date');

  /* ── Render parking areas list ── */
  renderAreaList();

  /* ── Render parking prices ── */
  renderPrices();

  /* ── Render active booking ── */
  renderActiveBooking();

  /* ── Simulate live availability update every 5 seconds ── */
  startLiveTick();
});

/* ── Render the area list on the left ── */
function renderAreaList() {
  const container = document.getElementById('dash-area-list');
  if (!container) return;

  container.innerHTML = PARKING_AREAS.map(area => `
    <div class="area-row">
      <div>
        <div class="area-name">${area.name}</div>
        <div class="area-sub">${area.location}</div>
      </div>
      <div class="area-count ${countColour(area.status)}">${area.available}</div>
    </div>
  `).join('');
}

/* ── Render price rows ── */
function renderPrices() {
  const container = document.getElementById('dash-prices');
  if (!container) return;

  container.innerHTML = PARKING_PRICES.map(p => `
    <div class="price-row">
      <span>${p.label}</span>
      <span class="price-amount">${p.amount}</span>
    </div>
  `).join('');
}

/* ── Render active booking card ── */
function renderActiveBooking() {
  const container = document.getElementById('dash-active-booking');
  if (!container) return;

  container.innerHTML = `
    <div class="active-booking-label">Active Booking</div>
    <div class="detail-row">
      <span class="detail-key">Zone</span>
      <span class="detail-val">${ACTIVE_BOOKING.zone}</span>
    </div>
    <div class="detail-row">
      <span class="detail-key">Date</span>
      <span class="detail-val">${ACTIVE_BOOKING.date}</span>
    </div>
    <div class="detail-row">
      <span class="detail-key">Time</span>
      <span class="detail-val">${ACTIVE_BOOKING.time}</span>
    </div>
    <div class="detail-row">
      <span class="detail-key">Registration</span>
      <span class="detail-val mono">${ACTIVE_BOOKING.reg}</span>
    </div>
  `;
}

/* ── Simulate live free space counter changing every 5s ── */
function startLiveTick() {
  setInterval(() => {
    const base = 230;
    const free = base + Math.floor(Math.random() * 40);
    const occ  = Math.round((1 - free / 450) * 100);

    const freeEl = document.getElementById('dash-free');
    const occEl  = document.getElementById('dash-occ');
    if (freeEl) freeEl.textContent = free;
    if (occEl)  occEl.textContent  = occ + '%';
  }, 5000);
}
