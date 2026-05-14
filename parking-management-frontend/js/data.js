/* ============================================================
   data.js — Shared fake/mock data for the entire application
   ============================================================ */

/* ── Parking Areas ── */
const PARKING_AREAS = [
  { id: 'A', name: 'Area A', location: 'Main Campus',          total: 200, available: 52,  status: 'available' },
  { id: 'B', name: 'Area B', location: 'Innovation Campus',    total: 200, available: 28,  status: 'limited'   },
  { id: 'C', name: 'Area C', location: 'Sports Complex',       total: 250, available: 62,  status: 'available' },
  { id: 'D', name: 'Area D', location: 'Library',              total: 200, available: 0,   status: 'full'      },
  { id: 'E', name: 'Area E', location: 'Student Residences',   total: 350, available: 0,   status: 'full'      },
  { id: 'F', name: 'Area F', location: 'Admin Building',       total: 180, available: 72,  status: 'available' },
];

/* ── Summary stats shown on dashboard ── */
const CAMPUS_STATS = {
  totalSpaces:     1200,
  occupiedSpaces:  847,
  availableSpaces: 353,
  occupancyRate:   '70.6%',
  freeSpaces:      247,
  campusOccupancy: '45%',
};

/* ── Parking prices ── */
const PARKING_PRICES = [
  { label: 'Hourly Rate', amount: '$3.00' },
  { label: 'Daily Rate',  amount: '$15.00' },
  { label: 'Weekly Pass', amount: '$50.00' },
];

/* ── Time slots available for booking ── */
const TIME_SLOTS = [
  '08:00 – 10:00',
  '09:00 – 17:00',
  '10:00 – 12:00',
  '12:00 – 14:00',
  '14:00 – 16:00',
  '16:00 – 18:00',
];

/* ── Bookable areas (have reserved sections) ── */
const BOOKABLE_AREAS = [
  { id: 'A', name: 'Parking Area A', location: 'Main Campus',       spaces: 12, status: 'available' },
  { id: 'B', name: 'Parking Area B', location: 'Innovation Campus', spaces: 8,  status: 'available' },
  { id: 'C', name: 'Parking Area C', location: 'Sports Complex',    spaces: 5,  status: 'limited'   },
  { id: 'D', name: 'Parking Area D', location: 'Library',           spaces: 0,  status: 'full'      },
];

/* ── Sample user bookings (mutable — modified by manage-bookings.js) ── */
let userBookings = [
  { id: 1, area: 'Parking Area A', date: '2026-04-15', time: '09:00 – 17:00', reg: 'ABC 123' },
  { id: 2, area: 'Zone B',         date: '2026-04-22', time: '08:00 – 16:00', reg: 'ABC 123' },
  { id: 3, area: 'Parking Area C', date: '2026-04-29', time: '09:00 – 17:00', reg: 'XYZ 789' },
];

/* ── Active booking shown on dashboard ── */
const ACTIVE_BOOKING = {
  zone: 'B12 – Innovation Campus',
  date: 'Today',
  time: '9:00 – 17:00',
  reg:  'ABC-123',
};

/* ── Admin: area summary ── */
const ADMIN_AREA_SUMMARY = [
  { area: 'Parking Area A', occupied: 180, available: 20,  status: 'high'   },
  { area: 'Parking Area B', occupied: 145, available: 55,  status: 'medium' },
  { area: 'Parking Area C', occupied: 220, available: 30,  status: 'high'   },
  { area: 'Parking Area D', occupied: 98,  available: 102, status: 'low'    },
  { area: 'Parking Area E', occupied: 204, available: 146, status: 'medium' },
];

/* ── Admin: peak hours ── */
const PEAK_HOURS = [
  { time: '08:00 – 09:00', pct: 92 },
  { time: '09:00 – 10:00', pct: 88 },
  { time: '10:00 – 11:00', pct: 75 },
  { time: '12:00 – 13:00', pct: 85 },
  { time: '17:00 – 18:00', pct: 78 },
  { time: '18:00 – 19:00', pct: 45 },
];

/* ── Admin: utilisation ── */
const UTILISATION = [
  { area: 'Parking Area A', pct: 90, level: 'hi'  },
  { area: 'Parking Area B', pct: 73, level: 'mid' },
  { area: 'Parking Area C', pct: 88, level: 'hi'  },
  { area: 'Parking Area D', pct: 49, level: 'lo'  },
  { area: 'Parking Area E', pct: 58, level: 'mid' },
];

/* ── Admin: weekly bar chart data ── */
const WEEKLY_USAGE = [
  { day: 'Mon', height: 72,  opacity: 0.75 },
  { day: 'Tue', height: 85,  opacity: 0.75 },
  { day: 'Wed', height: 100, opacity: 1    },
  { day: 'Thu', height: 78,  opacity: 0.75 },
  { day: 'Fri', height: 65,  opacity: 0.75 },
  { day: 'Sat', height: 30,  opacity: 0.4  },
  { day: 'Sun', height: 20,  opacity: 0.3  },
];

/* ── Security: occupancy vs paid ── */
const SECURITY_OCC_VS_PAID = [
  { area: 'Parking Area A', occupied: 42, paid: 42, diff: 0,  status: 'clear' },
  { area: 'Parking Area B', occupied: 38, paid: 35, diff: 3,  status: 'check' },
  { area: 'Parking Area C', occupied: 31, paid: 29, diff: 2,  status: 'check' },
  { area: 'Parking Area D', occupied: 16, paid: 13, diff: 3,  status: 'check' },
];

/* ── Security: unpaid parking table ── */
const UNPAID_PARKING = [
  { area: 'Area A – Main Campus',    occupied: 42, tickets: 42, timeOver: null },
  { area: 'Area B – Innovation',     occupied: 38, tickets: 35, timeOver: 14   },
  { area: 'Area C – Sports Complex', occupied: 31, tickets: 29, timeOver: 11   },
  { area: 'Area D – Library',        occupied: 16, tickets: 13, timeOver: 18   },
];

/* ── Security: active booking list ── */
const ACTIVE_BOOKINGS = [
  { reg: 'ABC 123', area: 'Parking Area B', time: '14:00 – 16:00', status: 'valid'    },
  { reg: 'XYZ 789', area: 'Parking Area B', time: '13:30 – 17:00', status: 'valid'    },
  { reg: 'DEF 456', area: 'Parking Area B', time: '12:00 – 18:00', status: 'valid'    },
  { reg: 'GHI 012', area: 'Parking Area B', time: '14:30 – 15:30', status: 'valid'    },
  { reg: 'JKL 345', area: 'Parking Area B', time: '14:00 – 16:30', status: 'conflict' },
  { reg: 'MNO 678', area: 'Parking Area A', time: '09:00 – 17:00', status: 'valid'    },
  { reg: 'PQR 901', area: 'Parking Area C', time: '10:00 – 14:00', status: 'valid'    },
];

/* ── Security: reserved space issues ── */
const RESERVED_ISSUES = [
  { area: 'Parking Area B', time: '14:15', type: 'Booking conflict',    status: 'review'   },
  { area: 'Parking Area C', time: '13:42', type: 'No space available',  status: 'resolved' },
];
