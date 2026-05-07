//JavaScript code

const channel = new MessageChannel();
let pollTimer = null;
startRealtime();

document.body.style.backgroundColor = localStorage.backgroundColor;
document.getElementsByName("radioGroup").forEach((radio) => {
  radio.checked = localStorage.getItem(radio.id) === "checked";
});

document.getElementById("myImage").onclick = function () {
  document.getElementById("Paragraph").innerHTML =
    "The image has been clicked!";
};

function submit(userInput) {
    channel.port1.postMessage("auth?" + document.getElementById("inputField").value);
}

document.getElementById("colorButton").onclick = function () {
  let userInput = document.getElementById("inputField").value;
  if (userInput) {
    document.body.style.backgroundColor = userInput;
    localStorage.backgroundColor = userInput;
    document.getElementsByName("radioGroup").forEach((radio) => {
      localStorage.setItem(radio.id, "");
      radio.checked = false;
    });
  }
};

document.getElementsByName("radioGroup").forEach((radio) => {
  radio.onchange = function () {
    document.getElementsByName("radioGroup").forEach((r) => {
      localStorage.setItem(r.id, r === radio ? "checked" : "");
    });
    document.body.style.backgroundColor = this.value;
    localStorage.backgroundColor = this.value;
  };
});

function greetUser(name) {
  return "Hello, " + name + "!";
}

console.log(greetUser("Student"));

// Login and realtime polling example
async function login() {
  const user = document.getElementById('loginUser').value;
  const pass = document.getElementById('loginPass').value;
  try {
    const base = location.protocol === 'file:' ? 'http://127.0.0.1:8080' : '';
    const resp = await fetch(base + '/api/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ username: user, password: pass })
    });
    if (!resp.ok) {
      const e = await resp.json();
      document.getElementById('loginStatus').textContent = e.error || 'login failed';
      document.getElementById('loginStatus').style.color = 'red';
      return;
    }
    document.getElementById('loginStatus').textContent = 'logged in';
    document.getElementById('loginStatus').style.color = 'green';
    startRealtime();
  } catch (err) {
    document.getElementById('loginStatus').textContent = 'network error';
    document.getElementById('loginStatus').style.color = 'red';
  }
}

function startRealtime() {
  if (pollTimer) return;
  const updates = document.getElementById('updates');
  // Try WebSocket first
  try {
    const wsProto = location.protocol === 'https:' ? 'wss' : 'ws';
    // when the page is opened via file://, location.host is empty -> fall back to localhost:8080
    const host = location.host || '127.0.0.1:8080';
    const url = `${wsProto}://${host}/ws`;
    const socket = new WebSocket(url);
    socket.onopen = () => { updates.textContent = 'ws connected'; };
    socket.onmessage = (ev) => {
      try {
        // server sends JSON with ts, value and counter
        const j = JSON.parse(ev.data);
        updates.textContent = `counter=${j.counter} ts=${j.ts} value=${j.value}`;
      } catch (e) {
        updates.textContent = ev.data;
      }
    };
    socket.onclose = () => {
      updates.textContent = 'ws closed, falling back to polling';
      startPolling();
    };
    socket.onerror = () => {
      updates.textContent = 'ws error, falling back to polling';
      startPolling();
    };
    return;
  } catch (e) {
    console.warn('websocket not available, fallback to polling', e);
    startPolling();
  }
  function startPolling() {
    pollTimer = setInterval(async () => {
      try {
        // when page is served from file:// fetch to relative paths will fail; use explicit localhost:8080
        const base = location.protocol === 'file:' ? 'http://127.0.0.1:8080' : '';
        const r = await fetch(base + '/api/updates');
        if (!r.ok) {
          if (r.status === 401) {
            updates.textContent = 'unauthenticated';
            clearInterval(pollTimer);
            pollTimer = null;
          }
          return;
        }
        const j = await r.json();
        updates.textContent = `counter=${j.counter} ts=${j.ts} value=${j.value}`;
      } catch (e) {
        console.error(e);
      }
    }, 1000);
  }
}
