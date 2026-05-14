/* ============================================================
   main.js — Shared utility functions used across all pages
   ============================================================ */

/* ── Live clock (dashboard & availability topbar) ── */
let clockTimer = null;

function startClock(elementId) {
  const el = document.getElementById(elementId);
  if (!el) return;
  const update = () => { el.textContent = new Date().toLocaleTimeString('en-AU'); };
  update();
  if (clockTimer) clearInterval(clockTimer);
  clockTimer = setInterval(update, 1000);
}

/* ── Toast notification ── */
function showToast(msg, icon = '') {
  const el = document.getElementById('toast-el');
  if (!el) return;
  document.getElementById('toast-icon').textContent = icon;
  document.getElementById('toast-msg').textContent  = msg;
  el.classList.add('show');
  setTimeout(() => el.classList.remove('show'), 2800);
}

/* ── Modal open / close ── */
function openModal(id) {
  const el = document.getElementById(id);
  if (el) el.classList.add('open');
}

function closeModal(id) {
  const el = document.getElementById(id);
  if (el) el.classList.remove('open');
}

/* ── Tab switcher (generic) ──
   tabPrefix: e.g. 'atab-' for admin, 'stab-' for security
   panePrefix: e.g. 'apane-' for admin, 'spane-' for security
   name: the tab name to activate
*/
function switchTab(tabPrefix, panePrefix, name) {
  document.querySelectorAll(`[id^="${tabPrefix}"]`).forEach(b => b.classList.remove('active'));
  document.querySelectorAll(`[id^="${panePrefix}"]`).forEach(p => p.classList.remove('active'));
  const btn  = document.getElementById(tabPrefix  + name);
  const pane = document.getElementById(panePrefix + name);
  if (btn)  btn.classList.add('active');
  if (pane) pane.classList.add('active');
}

/* ── Set today's date on a date input ── */
function setTodayDate(elementId) {
  const el = document.getElementById(elementId);
  if (el) el.value = new Date().toISOString().split('T')[0];
}

/* ── Get status tag HTML ── */
function statusTag(status) {
  const map = {
    available: 'tag-available',
    limited:   'tag-limited',
    full:      'tag-full',
    valid:     'tag-valid',
    conflict:  'tag-conflict',
    high:      'tag-full',
    medium:    'tag-limited',
    low:       'tag-available',
  };
  const cls  = map[status] || 'tag-limited';
  const label = status.charAt(0).toUpperCase() + status.slice(1);
  return `<span class="tag ${cls}">${label}</span>`;
}

/* ── Get text colour class for a parking count ── */
function countColour(status) {
  if (status === 'available') return 'text-green';
  if (status === 'limited')   return 'text-amber';
  if (status === 'full')      return 'text-crimson';
  return '';
}

/* ── Build sidebar HTML (shared across all app pages) ──
   activeLink: the key matching a nav item's data-page attribute
   role: 'user' | 'admin' | 'security'
*/
function buildSidebar(activeLink, role) {
  const userLinks = `
    <button class="nav-link ${activeLink === 'dashboard'    ? 'active' : ''}" onclick="location.href='dashboard.html'">
      <span class="ico">⊞</span>Dashboard
    </button>
    <button class="nav-link ${activeLink === 'availability' ? 'active' : ''}" onclick="location.href='availability.html'">
      <span class="ico">◎</span>Availability
    </button>
    <button class="nav-link ${activeLink === 'book'         ? 'active' : ''}" onclick="location.href='booking.html'">
      <span class="ico">＋</span>Book Parking
    </button>
    <button class="nav-link ${activeLink === 'manage'       ? 'active' : ''}" onclick="location.href='manage-bookings.html'">
      <span class="ico">☰</span>Manage Bookings
    </button>`;

  const adminLinks = `
    <div class="nav-section">Admin / Security</div>
    <button class="nav-link ${activeLink === 'admin'        ? 'active' : ''}" onclick="location.href='admin.html'">
      <span class="ico">⚙</span>Admin Dashboard
    </button>
    <button class="nav-link ${activeLink === 'security'     ? 'active' : ''}" onclick="location.href='security.html'">
      <span class="ico">🔒</span>Security Ops
    </button>`;

  const avatarClass = role === 'admin' ? 'avatar-admin' : role === 'security' ? 'avatar-security' : '';
  const userName    = role === 'admin' ? 'Admin User'   : role === 'security' ? 'Security Officer' : 'Jubair Hasan';
  const userRole    = role === 'admin' ? 'Administrator': role === 'security' ? 'Security'         : 'User';
  const initials    = role === 'admin' ? 'AD'           : role === 'security' ? 'SC'               : 'JH';
  const titleText   = role === 'admin' ? 'Admin Dashboard' : role === 'security' ? 'Security Dashboard' : 'Parking Dashboard';

  return `
    <div class="sidebar-header">
      <div class="sidebar-brand">UOW Parking</div>
      <div class="sidebar-title">${titleText}</div>
    </div>
    <nav class="sidebar-nav">
      ${userLinks}
      ${adminLinks}
    </nav>
    <div class="sidebar-footer">
      <div class="sidebar-user">
        <div class="avatar ${avatarClass}">${initials}</div>
        <div>
          <div class="sidebar-user-name">${userName}</div>
          <div class="sidebar-user-role">${userRole}</div>
        </div>
      </div>
      <button class="nav-link nav-link-logout" onclick="location.href='index.html'">
        <span class="ico">↩</span>Log Out
      </button>
    </div>`;
}

/* ── Inject sidebar into page ── */
function initSidebar(activeLink, role = 'user') {
  const container = document.getElementById('sidebar');
  if (container) container.innerHTML = buildSidebar(activeLink, role);
}

/* ── Build toast HTML (injected at bottom of body) ── */
function initToast() {
  const existing = document.getElementById('toast-el');
  if (existing) return;
  const toast = document.createElement('div');
  toast.className = 'toast';
  toast.id = 'toast-el';
  toast.innerHTML = '<span id="toast-icon"></span><span id="toast-msg"></span>';
  document.body.appendChild(toast);
}

/* ── Run on every page load ── */
document.addEventListener('DOMContentLoaded', () => {
  initToast();
});
