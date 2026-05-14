/* ============================================================
   manage-bookings.js — Logic for manage-bookings.html
   ============================================================ */

let selectedBookingId = null;
let pendingDeleteId   = null;

document.addEventListener('DOMContentLoaded', () => {

  /* ── Inject sidebar ── */
  initSidebar('manage', 'user');

  /* ── Set today's date on the change form ── */
  setTodayDate('ch-date');

  /* ── Render booking list ── */
  renderBookings();

  /* ── Update booking button ── */
  document.getElementById('update-booking-btn').addEventListener('click', updateBooking);

  /* ── Confirm delete button inside modal ── */
  document.getElementById('confirm-del-btn').addEventListener('click', confirmDelete);
});

/* ── Render the list of user bookings ── */
function renderBookings() {
  const container = document.getElementById('bookings-list');
  if (!container) return;

  if (!userBookings.length) {
    container.innerHTML = `
      <div style="padding:20px; text-align:center; color:var(--text-3); font-size:12px;">
        You currently have no active bookings.
      </div>`;
    clearDetailPanel();
    return;
  }

  container.innerHTML = userBookings.map(b => `
    <div class="booking-item" id="booking-item-${b.id}" onclick="selectBooking(${b.id})">
      <div class="booking-item-top">${b.area}</div>
      <div class="booking-item-meta">${b.date} &nbsp;·&nbsp; ${b.time} &nbsp;·&nbsp; ${b.reg}</div>
      <div class="booking-item-actions">
        <button class="btn btn-secondary btn-sm"
          onclick="event.stopPropagation(); selectBooking(${b.id})">
          Change
        </button>
        <button class="btn btn-danger btn-sm"
          onclick="event.stopPropagation(); openDeleteModal(${b.id})">
          Delete
        </button>
      </div>
    </div>
  `).join('');

  /* Auto-select first booking */
  if (userBookings.length) selectBooking(userBookings[0].id);
}

/* ── Select a booking to show in the detail panel ── */
function selectBooking(id) {
  selectedBookingId = id;
  const booking = userBookings.find(b => b.id === id);
  if (!booking) return;

  /* Highlight the selected item */
  document.querySelectorAll('.booking-item').forEach(el => {
    el.style.borderColor = 'var(--border)';
    el.style.background  = '#fff';
  });
  const selectedEl = document.getElementById(`booking-item-${id}`);
  if (selectedEl) {
    selectedEl.style.borderColor = 'var(--blue-bdr)';
    selectedEl.style.background  = 'var(--blue-pale)';
  }

  /* Populate detail panel */
  document.getElementById('md-area').textContent = booking.area;
  document.getElementById('md-date').textContent = booking.date;
  document.getElementById('md-time').textContent = booking.time;
  document.getElementById('md-reg').textContent  = booking.reg;
}

/* ── Clear detail panel when no bookings ── */
function clearDetailPanel() {
  ['md-area', 'md-date', 'md-time', 'md-reg'].forEach(id => {
    document.getElementById(id).textContent = '—';
  });
}

/* ── Handle Update Booking ── */
function updateBooking() {
  const booking = userBookings.find(b => b.id === selectedBookingId);
  if (!booking) { showToast('Please select a booking first.', '⚠️'); return; }

  const newDate = document.getElementById('ch-date').value;
  const newTime = document.getElementById('ch-time').value;
  const newArea = document.getElementById('ch-area').value;

  if (newDate) booking.date = newDate;
  if (newTime) booking.time = newTime;
  if (newArea) booking.area = newArea;

  renderBookings();
  showToast('Booking updated successfully.', '✅');
}

/* ── Open delete confirmation modal ── */
function openDeleteModal(id) {
  pendingDeleteId = id;
  const booking = userBookings.find(b => b.id === id);
  document.getElementById('del-modal-body').textContent =
    `Are you sure you want to delete your booking for ${booking.area} on ${booking.date}?`;
  openModal('del-modal');
}

/* ── Confirm and execute delete ── */
function confirmDelete() {
  userBookings = userBookings.filter(b => b.id !== pendingDeleteId);
  pendingDeleteId = null;
  closeModal('del-modal');
  renderBookings();
  showToast('Booking deleted.', '🗑️');
}
