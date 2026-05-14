/* ============================================================
   booking.js — Logic for booking.html
   ============================================================ */

let selectedArea = null; // tracks which area row the user clicked

document.addEventListener('DOMContentLoaded', () => {

  /* ── Inject sidebar ── */
  initSidebar('book', 'user');

  /* ── Set today's date ── */
  setTodayDate('bk-date');

  /* ── Populate time slot dropdown ── */
  populateTimeSlots();

  /* ── Populate filter dropdown ── */
  populateAreaFilter();

  /* ── Render bookable areas table ── */
  renderBookableAreas();

  /* ── Sync selected booking summary when date/time changes ── */
  document.getElementById('bk-date').addEventListener('change', syncSummary);
  document.getElementById('bk-time').addEventListener('change', syncSummary);

  /* ── Confirm booking ── */
  document.getElementById('confirm-booking-btn').addEventListener('click', confirmBooking);

  /* ── Book another (reset form) ── */
  document.getElementById('book-another-btn').addEventListener('click', resetForm);
});

/* ── Populate time slot <select> from TIME_SLOTS data ── */
function populateTimeSlots() {
  const select = document.getElementById('bk-time');
  if (!select) return;
  TIME_SLOTS.forEach(slot => {
    const opt = document.createElement('option');
    opt.value = slot;
    opt.textContent = slot;
    select.appendChild(opt);
  });
}

/* ── Populate area filter <select> ── */
function populateAreaFilter() {
  const select = document.getElementById('bk-filter');
  if (!select) return;
  BOOKABLE_AREAS.forEach(area => {
    const opt = document.createElement('option');
    opt.value = area.id;
    opt.textContent = area.name;
    select.appendChild(opt);
  });
}

/* ── Render the available areas table ── */
function renderBookableAreas() {
  const tbody = document.querySelector('#bk-areas-tbl tbody');
  if (!tbody) return;

  tbody.innerHTML = BOOKABLE_AREAS.map(area => {
    const isClickable = area.status !== 'full';
    const rowStyle    = isClickable ? 'cursor:pointer;' : 'color:var(--text-3);';

    return `
      <tr id="row-${area.id}" style="${rowStyle}"
        ${isClickable ? `onclick="pickArea('${area.id}', '${area.name}', '${area.location}', this)"` : ''}>
        <td>
          <div class="tbl-area">${area.name}</div>
          <div class="tbl-sub">${area.location}</div>
        </td>
        <td class="tbl-count ${isClickable ? countColour(area.status) : 'text-crimson'}">
          ${area.spaces}
        </td>
        <td>${statusTag(area.status)}</td>
      </tr>`;
  }).join('');
}

/* ── Called when user clicks an area row ── */
function pickArea(id, name, location, row) {
  selectedArea = { id, name, location };

  /* Highlight selected row */
  document.querySelectorAll('#bk-areas-tbl tr').forEach(r => r.style.background = '');
  row.style.background = 'var(--blue-pale)';

  /* Update summary panel */
  document.getElementById('sel-area').textContent = `${name} – ${location}`;
  syncSummary();
}

/* ── Sync date and time into the summary panel ── */
function syncSummary() {
  const date = document.getElementById('bk-date').value;
  const time = document.getElementById('bk-time').value;
  if (date) document.getElementById('sel-date').textContent = date;
  if (time) document.getElementById('sel-time').textContent = time;
}

/* ── Handle Confirm Booking ── */
function confirmBooking() {
  const area = document.getElementById('sel-area').textContent;
  const date = document.getElementById('sel-date').textContent;
  const time = document.getElementById('sel-time').textContent;
  const reg  = document.getElementById('bk-reg').value.trim();

  /* Validation */
  if (area === '[No area selected]') { showToast('Please select a parking area.', '⚠️'); return; }
  if (date === '[Not selected]')     { showToast('Please select a date.', '⚠️'); return; }
  if (time === '[Not selected]')     { showToast('Please select a time slot.', '⚠️'); return; }
  if (!reg)                          { showToast('Please enter your vehicle registration.', '⚠️'); return; }

  /* Add to shared bookings array (picked up by manage-bookings.js on that page) */
  userBookings.push({
    id:   Date.now(),
    area: area.split(' – ')[0],
    date,
    time,
    reg,
  });

  /* Build success detail block */
  document.getElementById('book-success-detail').innerHTML = `
    <div class="detail-row"><span class="detail-key">Parking Area</span><span class="detail-val">${area}</span></div>
    <div class="detail-row"><span class="detail-key">Date</span><span class="detail-val">${date}</span></div>
    <div class="detail-row"><span class="detail-key">Time Slot</span><span class="detail-val">${time}</span></div>
    <div class="detail-row"><span class="detail-key">Registration</span><span class="detail-val mono">${reg}</span></div>
    <div class="detail-row"><span class="detail-key">Type</span><span class="detail-val">Reserved Section</span></div>
  `;

  /* Show confirmation, hide form */
  document.getElementById('book-form').classList.add('hidden');
  document.getElementById('book-success').classList.remove('hidden');
}

/* ── Reset form to initial state ── */
function resetForm() {
  selectedArea = null;

  document.getElementById('sel-area').textContent = '[No area selected]';
  document.getElementById('sel-date').textContent = '[Not selected]';
  document.getElementById('sel-time').textContent = '[Not selected]';
  document.getElementById('bk-reg').value = '';
  document.getElementById('bk-time').value = '';
  setTodayDate('bk-date');

  document.querySelectorAll('#bk-areas-tbl tr').forEach(r => r.style.background = '');

  document.getElementById('book-success').classList.add('hidden');
  document.getElementById('book-form').classList.remove('hidden');
}
