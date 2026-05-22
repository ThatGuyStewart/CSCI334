(() => {
	let socket = null;
	let selectedLotId = null;
	let selectedBooking = null;
	let currentBookings = [];

	const logElement = document.getElementById("log");
	const wsStatusElement = document.getElementById("wsStatus");
	const lotGridElement = document.getElementById("lotGrid");
	const selectedLotElement = document.getElementById("selectedLot");

	const emailInput = document.getElementById("email");
	const passwordInput = document.getElementById("password");

	const loginButton = document.getElementById("loginButton");
	const userButton = document.getElementById("userButton");
	const updatesButton = document.getElementById("updatesButton");

	const connectWsButton = document.getElementById("connectWsButton");
	const wsLoginButton = document.getElementById("wsLoginButton");
	const disconnectWsButton = document.getElementById("disconnectWsButton");

	const bookingStartInput = document.getElementById("bookingStart");
	const bookingEndInput = document.getElementById("bookingEnd");
	const predictionTimeInput = document.getElementById("predictionTime");

	const registrationInput = document.getElementById("registration");
	const bookingsListElement = document.getElementById("bookingsList");
	const selectedBookingElement = document.getElementById("selectedBooking");
	const loadBookingsButton = document.getElementById("loadBookingsButton");
	const updateBookingButton = document.getElementById("updateBookingButton");
	const cancelBookingButton = document.getElementById("cancelBookingButton");

	const freeBookingsButton = document.getElementById("freeBookingsButton");
	const createBookingButton = document.getElementById("createBookingButton");
	const predictNormalButton = document.getElementById("predictNormalButton");
	const predictDisabledButton = document.getElementById("predictDisabledButton");
	const loadLotsButton = document.getElementById("loadLotsButton");

	function log(title, value) {
		const timestamp = new Date().toLocaleTimeString();
		let text = `[${timestamp}] ${title}`;

		if (value !== undefined) {
			try {
				text += `\n${JSON.stringify(value, null, 2)}`;
			}
			catch {
				text += `\n${String(value)}`;
			}
		}

		logElement.textContent = `${text}\n\n${logElement.textContent}`;
	}

	function credentials() {
		return {
			username: emailInput.value,
			password: passwordInput.value
		};
	}

	function updateSelectedLotLabel() {
		selectedLotElement.textContent = selectedLotId === null ? "None" : String(selectedLotId);
	}

	async function postJson(url, body) {
		const response = await fetch(url, {
			method: "POST",
			headers: {
				"Content-Type": "application/json"
			},
			body: JSON.stringify(body),
			credentials: "include"
		});

		const text = await response.text();

		try {
			return {
				status: response.status,
				ok: response.ok,
				body: JSON.parse(text)
			};
		}
		catch {
			return {
				status: response.status,
				ok: response.ok,
				body: text
			};
		}
	}

	async function getJson(url) {
		const response = await fetch(url, {
			method: "GET",
			credentials: "include"
		});

		const text = await response.text();

		try {
			return {
				status: response.status,
				ok: response.ok,
				body: JSON.parse(text)
			};
		}
		catch {
			return {
				status: response.status,
				ok: response.ok,
				body: text
			};
		}
	}

	async function putJson(url, body) {
		const response = await fetch(url, {
			method: "PUT",
			headers: {
				"Content-Type": "application/json"
			},
			body: JSON.stringify(body),
			credentials: "include"
		});

		const text = await response.text();

		try {
			return {
				status: response.status,
				ok: response.ok,
				body: JSON.parse(text)
			};
		}
		catch {
			return {
				status: response.status,
				ok: response.ok,
				body: text
			};
		}
	}

	async function deleteJson(url, body) {
		const response = await fetch(url, {
			method: "DELETE",
			headers: {
				"Content-Type": "application/json"
			},
			body: JSON.stringify(body),
			credentials: "include"
		});

		const text = await response.text();

		try {
			return {
				status: response.status,
				ok: response.ok,
				body: JSON.parse(text)
			};
		}
		catch {
			return {
				status: response.status,
				ok: response.ok,
				body: text
			};
		}
	}

	function mergeLotData(normalLots, disabledLots) {
		const lots = new Map();

		for (const lot of normalLots) {
			if (!lots.has(lot.lotId)) {
				lots.set(lot.lotId, {
					lotId: lot.lotId,
					normalSpaces: [],
					disabledSpaces: []
				});
			}

			lots.get(lot.lotId).normalSpaces = lot.spaces ?? [];
		}

		for (const lot of disabledLots) {
			if (!lots.has(lot.lotId)) {
				lots.set(lot.lotId, {
					lotId: lot.lotId,
					normalSpaces: [],
					disabledSpaces: []
				});
			}

			lots.get(lot.lotId).disabledSpaces = lot.spaces ?? [];
		}

		return Array.from(lots.values()).sort((a, b) => a.lotId - b.lotId);
	}

	function createSpaceElement(space, extraClass = "") {
		const element = document.createElement("div");
		element.className = `space ${space.available ? "available" : "unavailable"} ${extraClass}`.trim();
		element.textContent = space.spaceId;
		element.title = `Space ${space.spaceId} - ${space.available ? "Available" : "Unavailable"}`;
		return element;
	}

	function renderSpaceSection(title, spaces, extraClass = "") {
		const section = document.createElement("div");
		section.className = "space-section";

		const heading = document.createElement("h3");
		heading.textContent = title;
		section.appendChild(heading);

		const grid = document.createElement("div");
		grid.className = "space-grid";

		const sortedSpaces = [...spaces].sort((a, b) => a.spaceId - b.spaceId);
		for (const space of sortedSpaces) {
			grid.appendChild(createSpaceElement(space, extraClass));
		}

		section.appendChild(grid);
		return section;
	}

	function selectLot(lotId) {
		selectedLotId = lotId;
		updateSelectedLotLabel();

		const cards = lotGridElement.querySelectorAll(".lot-card");
		for (const card of cards) {
			const cardLotId = Number(card.dataset.lotId);
			card.classList.toggle("selected", cardLotId === selectedLotId);
		}
	}

	function renderLots(lots) {
		lotGridElement.innerHTML = "";

		if (!lots.length) {
			lotGridElement.textContent = "No lot data available.";
			return;
		}

		for (const lot of lots) {
			const card = document.createElement("div");
			card.className = "lot-card";
			card.dataset.lotId = String(lot.lotId);

			if (selectedLotId === lot.lotId) {
				card.classList.add("selected");
			}

			card.addEventListener("click", () => {
				selectLot(lot.lotId);
			});

			const title = document.createElement("h3");
			title.textContent = `Lot ${lot.lotId}`;
			card.appendChild(title);

			card.appendChild(renderSpaceSection("Normal Spaces", lot.normalSpaces));
			card.appendChild(renderSpaceSection("Disabled Spaces", lot.disabledSpaces, "disabled"));

			lotGridElement.appendChild(card);
		}
	}

	function applyAvailabilityMessage(message) {
		const lots = mergeLotData(message.normalLots ?? [], message.disabledLots ?? []);
		renderLots(lots);

		if (selectedLotId !== null && !lots.some((lot) => lot.lotId === selectedLotId)) {
			selectedLotId = null;
			updateSelectedLotLabel();
		}
	}

	function connectWebSocket() {
		if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
			return;
		}

		const protocol = window.location.protocol === "https:" ? "wss" : "ws";
		socket = new WebSocket(`${protocol}://${window.location.host}/ws`);
		wsStatusElement.textContent = "Connecting";

		socket.addEventListener("open", () => {
			wsStatusElement.textContent = "Connected";
			log("WebSocket connected.");
		});

		socket.addEventListener("message", (event) => {
			try {
				const message = JSON.parse(event.data);

				if (message.type === "availability") {
					applyAvailabilityMessage(message);
					return;
				}

				if (message.type === "bookings") {
					applyBookingsMessage(message);
					return;
				}

				log("WebSocket message", message);
			}
			catch {
				log("WebSocket message", event.data);
			}
		});

		socket.addEventListener("close", () => {
			wsStatusElement.textContent = "Disconnected";
			log("WebSocket disconnected.");
		});

		socket.addEventListener("error", () => {
			wsStatusElement.textContent = "Error";
			log("WebSocket error.");
		});
	}

	function reconnectWebSocket() {
		if (socket) {
			socket.close();
			socket = null;
		}

		connectWebSocket();
	}

	function toEpochSeconds(value) {
		if (!value) {
			return null;
		}

		const date = new Date(value);
		if (Number.isNaN(date.getTime())) {
			return null;
		}

		return Math.floor(date.getTime() / 1000);
	}

	async function loadLotAvailability() {
		const [normalResponse, disabledResponse] = await Promise.all([
			getJson("/api/carpark/available/normal"),
			getJson("/api/carpark/available/disabled")
		]);

		if (!normalResponse.ok || !disabledResponse.ok) {
			log("Load lot availability failed", {
				normal: normalResponse.body,
				disabled: disabledResponse.body
			});
			return;
		}

		const lots = mergeLotData(
			normalResponse.body.lots ?? [],
			disabledResponse.body.lots ?? []);

		renderLots(lots);

		if (selectedLotId !== null && !lots.some((lot) => lot.lotId === selectedLotId)) {
			selectedLotId = null;
			updateSelectedLotLabel();
		}
	}

	loginButton.addEventListener("click", async () => {
		try {
			const result = await postJson("/api/login", credentials());
			log("HTTP login result", result);

			if (result.ok) {
				await loadLotAvailability();
				await loadUpcomingBookings();
				reconnectWebSocket();
			}
		}
		catch (error) {
			log("HTTP login failed", String(error));
		}
	});

	userButton.addEventListener("click", async () => {
		try {
			const result = await getJson("/api/user");
			log("Current user result", result);
		}
		catch (error) {
			log("Get user failed", String(error));
		}
	});

	updatesButton.addEventListener("click", async () => {
		try {
			const result = await getJson("/api/updates");
			log("Updates result", result);
		}
		catch (error) {
			log("Get updates failed", String(error));
		}
	});

	connectWsButton.addEventListener("click", () => {
		connectWebSocket();
	});

	wsLoginButton.addEventListener("click", () => {
		if (!socket || socket.readyState !== WebSocket.OPEN) {
			log("WebSocket is not connected.");
			return;
		}

		const body = {
			type: "login",
			username: emailInput.value,
			password: passwordInput.value
		};

		socket.send(JSON.stringify(body));
		log("WebSocket login sent", body);
	});

	disconnectWsButton.addEventListener("click", () => {
		if (socket) {
			socket.close();
			socket = null;
		}
	});
	freeBookingsButton.addEventListener("click", async () => {
		const startTime = toEpochSeconds(bookingStartInput.value);
		const endTime = toEpochSeconds(bookingEndInput.value);

		if (startTime === null || endTime === null) {
			log("Find available reserved lots failed", "Start and end times are required.");
			return;
		}

		if (endTime <= startTime) {
			log("Find available reserved lots failed", "End time must be after start time.");
			return;
		}

		try {
			const result = await getJson(
				`/api/carpark/available-reserved-lots?startTime=${startTime}&endTime=${endTime}`);

			log("Find available reserved lots result", result);

			if (!result.ok) {
				return;
			}

			const lotIds = result.body.lotIds ?? [];

			if (!lotIds.length) {
				selectedLotId = null;
				updateSelectedLotLabel();
				log("Find available reserved lots", "No reserved lots are available for that time range.");
				return;
			}

			if (!lotIds.includes(selectedLotId)) {
				selectLot(lotIds[0]);
			}

			log("Available reserved lots", lotIds);
		}
		catch (error) {
			log("Find available reserved lots failed", String(error));
		}
	});

	createBookingButton.addEventListener("click", async () => {
		const startTime = toEpochSeconds(bookingStartInput.value);
		const endTime = toEpochSeconds(bookingEndInput.value);

		if (selectedLotId === null) {
			showPopupMessage("Create Booking Failed", "Select a lot first.");
			log("Create booking failed", "Select a lot first.");
			return;
		}

		if (!registrationInput.value.trim()) {
			showPopupMessage("Create Booking Failed", "Registration is required.");
			log("Create booking failed", "Registration is required.");
			return;
		}

		if (startTime === null || endTime === null) {
			showPopupMessage("Create Booking Failed", "Start and end times are required.");
			log("Create booking failed", "Start and end times are required.");
			return;
		}

		const payload = {
			registration: registrationInput.value.trim(),
			lotId: selectedLotId,
			startTime,
			endTime
		};

		try {
			const result = await postJson("/api/carpark/bookings", payload);
			log("Create booking result", result);

			if (!result.ok) {
				showPopupMessage(
					"Create Booking Failed",
					getErrorMessage(result, "Booking could not be created."));
				return;
			}

			await loadLotAvailability();
			await loadUpcomingBookings();
		}
		catch (error) {
			showPopupMessage("Create Booking Failed", String(error));
			log("Create booking failed", String(error));
		}
	});

	function getBookingKey(booking) {
		return `${booking.lotId}|${booking.registration}|${booking.startTime}|${booking.endTime}`;
	}

	function updateSelectedBookingLabel() {
		if (!selectedBooking) {
			selectedBookingElement.textContent = "None";
			return;
		}

		selectedBookingElement.textContent =
			`Lot ${selectedBooking.lotId}, ${selectedBooking.registration}`;
	}

	function selectBooking(booking) {
		selectedBooking = {
			...booking,
			key: getBookingKey(booking)
		};
		selectedLotId = booking.lotId;

		registrationInput.value = booking.registration;
		bookingStartInput.value = toDateTimeLocalValue(booking.startTime);
		bookingEndInput.value = toDateTimeLocalValue(booking.endTime);

		updateSelectedLotLabel();
		updateSelectedBookingLabel();
		renderBookings(currentBookings);
	}

	function renderBookings(bookings) {
		bookingsListElement.innerHTML = "";

		if (!bookings.length) {
			bookingsListElement.textContent = "No upcoming bookings.";
			return;
		}

		for (const booking of bookings) {
			const element = document.createElement("div");
			element.className = "booking-card";

			if (selectedBooking && selectedBooking.key === getBookingKey(booking)) {
				element.classList.add("selected");
			}

			element.innerHTML = `
				<div><strong>Lot ${booking.lotId}</strong></div>
				<div>Registration: ${booking.registration}</div>
				<div>Start: ${new Date(booking.startTime * 1000).toLocaleString()}</div>
				<div>End: ${new Date(booking.endTime * 1000).toLocaleString()}</div>
			`;

			element.addEventListener("click", () => {
				selectBooking(booking);
			});

			bookingsListElement.appendChild(element);
		}
	}

	async function loadUpcomingBookings() {
		const result = await getJson("/api/bookings");
		if (!result.ok) {
			log("Load bookings failed", result);
			return;
		}

		currentBookings = result.body.bookings ?? [];

		if (selectedBooking) {
			// Ensure the selected booking is still in the upcoming bookings list
			const matchingBooking = currentBookings.find((booking) =>
				getBookingKey(booking) === selectedBooking.key);

			selectedBooking = matchingBooking
				? { ...matchingBooking, key: getBookingKey(matchingBooking) }
				: null;
		}

		updateSelectedBookingLabel();
		renderBookings(currentBookings);
	}

	loadBookingsButton.addEventListener("click", async () => {
		try {
			await loadUpcomingBookings();
		}
		catch (error) {
			log("Load bookings failed", String(error));
		}
	});

	updateBookingButton.addEventListener("click", async () => {
		const startTime = toEpochSeconds(bookingStartInput.value);
		const endTime = toEpochSeconds(bookingEndInput.value);

		if (!selectedBooking) {
			showPopupMessage("Update Booking Failed", "Select a booking first.");
			log("Update booking failed", "Select a booking first.");
			return;
		}

		if (selectedLotId === null) {
			showPopupMessage("Update Booking Failed", "Select a lot first.");
			log("Update booking failed", "Select a lot first.");
			return;
		}

		if (!registrationInput.value.trim()) {
			showPopupMessage("Update Booking Failed", "Registration is required.");
			log("Update booking failed", "Registration is required.");
			return;
		}

		if (startTime === null || endTime === null) {
			showPopupMessage("Update Booking Failed", "Start and end times are required.");
			log("Update booking failed", "Start and end times are required.");
			return;
		}

		const payload = {
			originalLotId: selectedBooking.lotId,
			originalRegistration: selectedBooking.registration,
			originalStartTime: selectedBooking.startTime,
			originalEndTime: selectedBooking.endTime,
			lotId: selectedLotId,
			registration: registrationInput.value.trim(),
			startTime,
			endTime
		};

		try {
			const result = await putJson("/api/bookings", payload);
			log("Update booking result", result);

			if (!result.ok) {
				showPopupMessage(
					"Update Booking Failed",
					getErrorMessage(result, "Booking could not be updated."));
				return;
			}

			selectedBooking = null;
			updateSelectedBookingLabel();
			await loadLotAvailability();
			await loadUpcomingBookings();
		}
		catch (error) {
			showPopupMessage("Update Booking Failed", String(error));
			log("Update booking failed", String(error));
		}
	});

	cancelBookingButton.addEventListener("click", async () => {
		if (!selectedBooking) {
			log("Cancel booking failed", "Select a booking first.");
			return;
		}

		const payload = {
			lotId: selectedBooking.lotId,
			registration: selectedBooking.registration,
			startTime: selectedBooking.startTime,
			endTime: selectedBooking.endTime
		};

		try {
			const result = await deleteJson("/api/bookings", payload);
			log("Cancel booking result", result);

			if (result.ok) {
				selectedBooking = null;
				updateSelectedBookingLabel();
				await loadLotAvailability();
				await loadUpcomingBookings();
			}
		}
		catch (error) {
			log("Cancel booking failed", String(error));
		}
	});

	predictNormalButton.addEventListener("click", async () => {
		const futureTime = toEpochSeconds(predictionTimeInput.value);

		if (futureTime === null) {
			log("Predict normal failed", "Prediction time is required.");
			return;
		}

		try {
			const result = await getJson(`/api/carpark/predict/normal?futureTime=${futureTime}`);
			log("Predict normal result", result);
		}
		catch (error) {
			log("Predict normal failed", String(error));
		}
	});

	predictDisabledButton.addEventListener("click", async () => {
		const futureTime = toEpochSeconds(predictionTimeInput.value);

		if (futureTime === null) {
			log("Predict disabled failed", "Prediction time is required.");
			return;
		}

		try {
			const result = await getJson(`/api/carpark/predict/disabled?futureTime=${futureTime}`);
			log("Predict disabled result", result);
		}
		catch (error) {
			log("Predict disabled failed", String(error));
		}
	});

	loadLotsButton.addEventListener("click", async () => {
		await loadLotAvailability();
	});

	updateSelectedLotLabel();
	updateSelectedBookingLabel();

	loadLotAvailability().catch((error) => {
		log("Initial availability load failed", error?.message ?? error);
	});

	connectWebSocket();


	function getErrorMessage(result, fallbackMessage) {
		if (!result || result.body === undefined || result.body === null) {
			return fallbackMessage;
		}

		if (typeof result.body === "string") {
			return result.body || fallbackMessage;
		}

		if (result.body.message) {
			return result.body.message;
		}

		if (result.body.error) {
			return result.body.error;
		}

		return fallbackMessage;
	}

	function showPopupMessage(title, message) {
		window.alert(`${title}\n\n${message}`);
	}

	function applyBookingsMessage(message) {
		currentBookings = message.bookings ?? [];

		if (selectedBooking) {
			const matchingBooking = currentBookings.find((booking) =>
				getBookingKey(booking) === selectedBooking.key);

			selectedBooking = matchingBooking
				? { ...matchingBooking, key: getBookingKey(matchingBooking) }
				: null;
		}

		updateSelectedBookingLabel();
		renderBookings(currentBookings);
	}
})();