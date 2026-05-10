(() => {
    const TZ = "UTC";
    const loginPanel = document.getElementById("loginPanel");
    const dashboard = document.getElementById("dashboard");
    const emailInput = document.getElementById("email");
    const passwordInput = document.getElementById("password");
    const loginButton = document.getElementById("loginButton");
    const refreshDashboardButton = document.getElementById("refreshDashboardButton");
    const adminUserElement = document.getElementById("adminUser");
    const graphsElement = document.getElementById("graphs");
    const lotSelectorElement = document.getElementById("lotSelector");
    const showBookingsButton = document.getElementById("showBookingsButton");
    const bookingsTableBody = document.getElementById("bookingsTableBody");
    const range24hButton = document.getElementById("range24hButton");
    const rangeWeekButton = document.getElementById("rangeWeekButton");
    const rangeMonthButton = document.getElementById("rangeMonthButton");
    const graphCenterTimeInput = document.getElementById("graphCenterTime");
    const browserCurrentTimeElement = document.getElementById("browserCurrentTime");
    const serverCurrentTimeElement = document.getElementById("serverCurrentTime");

    let allActivityLots = [];
    let allParkedWithoutTicketLots = [];
    let selectedRange = "24h";

    function toEpochSeconds(value) {
        if (!value) return null;

        const date = new Date(value);
        if (Number.isNaN(date.getTime())) return null;

        return Math.floor(date.getTime() / 1000);
    }

    function toDateTimeLocalValue(epochSeconds) {
        const date = new Date(epochSeconds * 1000);
        const year = date.getFullYear();
        const month = String(date.getMonth() + 1).padStart(2, "0");
        const day = String(date.getDate()).padStart(2, "0");
        const hours = String(date.getHours()).padStart(2, "0");
        const minutes = String(date.getMinutes()).padStart(2, "0");
        return `${year}-${month}-${day}T${hours}:${minutes}`;
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
            return { ok: response.ok, status: response.status, body: JSON.parse(text) };
        }
        catch {
            return { ok: response.ok, status: response.status, body: text };
        }
    }

    async function getJson(url) {
        const response = await fetch(url, {
            method: "GET",
            credentials: "include"
        });

        const text = await response.text();

        try {
            return { ok: response.ok, status: response.status, body: JSON.parse(text) };
        }
        catch {
            return { ok: response.ok, status: response.status, body: text };
        }
    }

    function showError(message) {
        window.alert(message);
    }

    function setLoggedIn(user) {
        adminUserElement.textContent = user;
        loginPanel.classList.add("hidden");
        dashboard.classList.remove("hidden");

        if (!graphCenterTimeInput.value) {
            graphCenterTimeInput.value = toDateTimeLocalValue(Math.floor(Date.now() / 1000));
        }

        updateBrowserCurrentTime();
        console.log(graphCenterTimeInput.value);
        console.log(toEpochSeconds(graphCenterTimeInput.value));
        console.log(new Date(toEpochSeconds(graphCenterTimeInput.value) * 1000).toISOString());
    }

    function setLoggedOut() {
        adminUserElement.textContent = "None";
        dashboard.classList.add("hidden");
        loginPanel.classList.remove("hidden");
    }

    function refreshLotSelector(lotIds) {
        const currentValue = Number(lotSelectorElement.value);
        lotSelectorElement.innerHTML = "";

        if (!lotIds.length) {
            const option = document.createElement("option");
            option.value = "";
            option.textContent = "No lots available";
            lotSelectorElement.appendChild(option);
            return;
        }

        for (const lotId of lotIds) {
            const option = document.createElement("option");
            option.value = String(lotId);
            option.textContent = `Lot ${lotId}`;
            lotSelectorElement.appendChild(option);
        }

        if (lotIds.includes(currentValue)) {
            lotSelectorElement.value = String(currentValue);
        }
        else {
            lotSelectorElement.value = String(lotIds[0]);
        }
    }

    function getCenterEpochSeconds() {
        return toEpochSeconds(graphCenterTimeInput.value) ?? Math.floor(Date.now() / 1000);
    }
    function getRangeWindowSeconds(range) {
        switch (range) {
            case "24h":
                return 24 * 60 * 60;
            case "week":
                return 7 * 24 * 60 * 60;
            case "month":
                return 30 * 24 * 60 * 60;
            default:
                return 24 * 60 * 60;
        }
    }

    function filterActivityLotsByRange(activityLots, range) {
    const center = getCenterEpochSeconds();
    const halfWindow = Math.floor(getRangeWindowSeconds(range) / 2);
    const rangeStart = center - halfWindow;
    const rangeEnd = center + halfWindow;

    return (activityLots ?? []).map((lot) => ({
        ...lot,
        rangeStart,
        rangeEnd,
        points: (lot.points ?? []).filter(
            (p) => p.time >= rangeStart && p.time <= rangeEnd
        )
    }));
    }


    function updateRangeButtons() {
        range24hButton.disabled = selectedRange === "24h";
        rangeWeekButton.disabled = selectedRange === "week";
        rangeMonthButton.disabled = selectedRange === "month";
    }

    function formatTimeLabel(epochSeconds) {
        const date = new Date(epochSeconds * 1000);

        switch (selectedRange) {
            case "24h":
                return date.toLocaleString([], {
                    day: "numeric",
                    month: "short",
                    hour: "2-digit",
                    minute: "2-digit"
                });
            case "week":
                return date.toLocaleString([], {
                    day: "numeric",
                    month: "short",
                    hour: "2-digit",
                    minute: "2-digit"
                });
            case "month":
                return date.toLocaleDateString([], {
                    day: "numeric",
                    month: "short"
                });
            default:
                return date.toLocaleString();
        }
    }

    function buildTimeScale(rangeStart, rangeEnd) {
        const middle = Math.floor((rangeStart + rangeEnd) / 2);

        return [
            formatTimeLabel(rangeStart),
            formatTimeLabel(middle),
            formatTimeLabel(rangeEnd)
        ];
    }

    function getXPosition(width, pointTime, rangeStart, rangeEnd) {
        if (rangeEnd <= rangeStart) {
            return width / 2;
        }

        const progress = (pointTime - rangeStart) / (rangeEnd - rangeStart);
        const clamped = Math.max(0, Math.min(1, progress));
        return 20 + (clamped * (width - 30));
    }

    function drawSeries(ctx, width, height, points, maxY, color, selector, rangeStart, rangeEnd, dashed = false) {
        if (!points.length || maxY <= 0) return;

        const sortedPoints = [...points]
            .filter((point) => Number.isFinite(point.time) && Number.isFinite(selector(point)))
            .sort((a, b) => a.time - b.time);

        if (!sortedPoints.length) {
            return;
        }

        ctx.beginPath();
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.setLineDash(dashed ? [6, 4] : []);

        sortedPoints.forEach((point, index) => {
            const x = getXPosition(width, point.time, rangeStart, rangeEnd);
            const y = height - 20 - ((selector(point) / maxY) * (height - 40));

            if (!Number.isFinite(x) || !Number.isFinite(y)) {
                return;
            }

            if (index === 0) {
                ctx.moveTo(x, y);
            }
            else {
                ctx.lineTo(x, y);
            }
        });

        ctx.stroke();
        ctx.setLineDash([]);
    }

    function drawSingleSeriesChart(
        canvas,
        points,
        actualColor,
        actualSelector,
        totalSpaces,
        rangeStart,
        rangeEnd) {
        const width = canvas.width;
        const height = canvas.height;
        const ctx = canvas.getContext("2d");

        ctx.clearRect(0, 0, width, height);
        ctx.fillStyle = "#ffffff";
        ctx.fillRect(0, 0, width, height);

        ctx.strokeStyle = "#d1d5db";
        ctx.lineWidth = 1;

        ctx.beginPath();
        ctx.moveTo(20, 10);
        ctx.lineTo(20, height - 20);
        ctx.lineTo(width - 10, height - 20);
        ctx.stroke();

        if (!points.length) {
            ctx.fillStyle = "#6b7280";
            ctx.font = "14px Arial";
            ctx.fillText("No data for selected range.", 24, 36);
            return;
        }

        const maxY = Math.max(1, totalSpaces);

        drawSeries(
            ctx,
            width,
            height,
            points,
            maxY,
            actualColor,
            actualSelector,
            rangeStart,
            rangeEnd,
            false);
    }

    function buildVerticalScale(totalSpaces) {
        return [
            String(totalSpaces),
            String(Math.round(totalSpaces / 2)),
            "0"
        ];
    }

    function createChartBlock(
        titleText,
        actualColor,
        points,
        actualSelector,
        totalSpaces,
        rangeStart,
        rangeEnd) {
        const block = document.createElement("div");
        block.className = "chart-block";

        const title = document.createElement("div");
        title.className = "chart-title";
        title.textContent = titleText;
        block.appendChild(title);

        const chartRow = document.createElement("div");
        chartRow.className = "chart-row";

        const yScale = document.createElement("div");
        yScale.className = "y-scale";

        for (const value of buildVerticalScale(totalSpaces)) {
            const label = document.createElement("span");
            label.textContent = value;
            yScale.appendChild(label);
        }

        chartRow.appendChild(yScale);

        const canvas = document.createElement("canvas");
        canvas.width = 520;
        canvas.height = 220;
        chartRow.appendChild(canvas);

        block.appendChild(chartRow);

        drawSingleSeriesChart(
            canvas,
            points,
            actualColor,
            actualSelector,
            totalSpaces,
            rangeStart,
            rangeEnd);

        const timeScale = document.createElement("div");
        timeScale.className = "time-scale";

        for (const labelText of buildTimeScale(rangeStart, rangeEnd)) {
            const label = document.createElement("span");
            label.textContent = labelText;
            timeScale.appendChild(label);
        }

        block.appendChild(timeScale);
        return block;
    }

    function renderGraphs(activityLots, parkedWithoutTicketLots) {
        graphsElement.innerHTML = "";

        const parkedMap = new Map(
            (parkedWithoutTicketLots ?? []).map((lot) => [lot.lotId, lot.parkedWithoutTicket]));

        const lotIds = activityLots.map((lot) => lot.lotId);
        refreshLotSelector(lotIds);

        for (const lot of activityLots) {
            const card = document.createElement("div");
            card.className = "lot-card";

            const title = document.createElement("h3");
            title.textContent = `Lot ${lot.lotId}`;
            card.appendChild(title);

            const parked = document.createElement("div");
            parked.textContent = `Parked without ticket: ${parkedMap.get(lot.lotId) ?? 0}`;
            card.appendChild(parked);

            const occupancyLabel = document.createElement("div");
            occupancyLabel.className = "occupancy-label";
            occupancyLabel.textContent = "Occupancy:";
            card.appendChild(occupancyLabel);

            const chartGroup = document.createElement("div");
            chartGroup.className = "chart-group";

            const points = lot.points ?? [];
            const rangeStart = lot.rangeStart ?? getCenterEpochSeconds();
            const rangeEnd = lot.rangeEnd ?? getCenterEpochSeconds();

            const actualColor = "#3b63fb";

            chartGroup.appendChild(
                createChartBlock(
                    "Normal Spaces",
                    actualColor,
                    points,
                    (point) => point.normalOccupied,
                    lot.normalTotal ?? 0,
                    rangeStart,
                    rangeEnd));

            chartGroup.appendChild(
                createChartBlock(
                    "Disabled Spaces",
                    actualColor,
                    points,
                    (point) => point.disabledOccupied,
                    lot.disabledTotal ?? 0,
                    rangeStart,
                    rangeEnd));

            chartGroup.appendChild(
                createChartBlock(
                    "Reserved Spaces",
                    actualColor,
                    points,
                    (point) => point.reservedOccupied,
                    lot.reservedTotal ?? 0,
                    rangeStart,
                    rangeEnd));

            card.appendChild(chartGroup);

            if (points.length) {
                const debug = document.createElement("div");
                debug.style.fontSize = "0.85rem";
                debug.style.color = "#6b7280";
                debug.textContent =
                    `First point: ${new Date(points[0].time * 1000).toLocaleString()} | ` +
                    `Last point: ${new Date(points[points.length - 1].time * 1000).toLocaleString()}`;
                card.appendChild(debug);
            }

            graphsElement.appendChild(card);
        }
    }

    function renderFilteredGraphs() {
        updateRangeButtons();
        renderGraphs(
            filterActivityLotsByRange(allActivityLots, selectedRange),
            allParkedWithoutTicketLots);
    }

    function renderBookings(bookings) {
        bookingsTableBody.innerHTML = "";

        if (!bookings.length) {
            bookingsTableBody.innerHTML = `<tr><td colspan="5">No current bookings for this lot.</td></tr>`;
            return;
        }

        for (const booking of bookings) {
            const row = document.createElement("tr");
            row.innerHTML = `
                <td>${booking.lotId}</td>
                <td>${booking.email}</td>
                <td>${booking.registration}</td>
                <td>${new Date(booking.startTime * 1000).toLocaleString()}</td>
                <td>${new Date(booking.endTime * 1000).toLocaleString()}</td>
            `;
            bookingsTableBody.appendChild(row);
        }
    }

    async function loadDashboard() {
        updateBrowserCurrentTime();
        await updateServerCurrentTime();

        const center = getCenterEpochSeconds();
        const halfWindow = Math.floor(getRangeWindowSeconds(selectedRange) / 2);
        const startTime = center - halfWindow;
        const endTime = center + halfWindow;

        const [activityResult, parkedResult] = await Promise.all([
            getJson(`/api/admin/lot-activity?startTime=${startTime}&endTime=${endTime}`),
            getJson("/api/admin/parked-without-ticket")
        ]);

        console.log("Admin lot activity result", activityResult);
        console.log("Admin parked-without-ticket result", parkedResult);

        if (!activityResult.ok) {
            allActivityLots = [];
            allParkedWithoutTicketLots = [];
            graphsElement.innerHTML = "<div>No lot activity data available.</div>";
            refreshLotSelector([]);
            throw new Error(
                typeof activityResult.body === "object"
                    ? (activityResult.body?.error ?? "Failed to load lot activity.")
                    : "Failed to load lot activity.");
        }

        if (!parkedResult.ok) {
            allParkedWithoutTicketLots = [];
        }
        else {
            allParkedWithoutTicketLots = parkedResult.body?.lots ?? [];
        }

        allActivityLots = activityResult.body?.lots ?? [];
        renderFilteredGraphs();
    }

    async function loadCurrentBookings() {
        const lotId = Number(lotSelectorElement.value);
        if (!lotId) {
            showError("Select a lot first.");
            return;
        }

        const result = await getJson(`/api/admin/current-bookings?lotId=${lotId}`);
        if (!result.ok) {
            showError(result.body?.error ?? "Failed to load current bookings.");
            return;
        }

        renderBookings(result.body.bookings ?? []);
    }

    async function restoreSession() {
        const result = await getJson("/api/admin/user");
        if (!result.ok) {
            setLoggedOut();
            return;
        }

        const serverTime = result.body.serverTime; // epoch seconds from server

        // Use server time, not browser time, for the graph center
        if (!graphCenterTimeInput.value && serverTime) {
            graphCenterTimeInput.value = toDateTimeLocalValue(serverTime);
        }

        setLoggedIn(result.body.user);
        await loadDashboard();
    }

    loginButton.addEventListener("click", async () => {
        const result = await postJson("/api/admin/login", {
            username: emailInput.value.trim(),
            password: passwordInput.value
        });

        if (!result.ok) {
            showError(result.body?.error ?? "Admin login failed.");
            return;
        }

        setLoggedIn(result.body.user);
        await loadDashboard();
    });

    refreshDashboardButton.addEventListener("click", async () => {
        try {
            await loadDashboard();
        }
        catch (error) {
            showError(error.message ?? String(error));
        }
    });

    showBookingsButton.addEventListener("click", async () => {
        try {
            await loadCurrentBookings();
        }
        catch (error) {
            showError(error.message ?? String(error));
        }
    });

    range24hButton.addEventListener("click", async () => {
        selectedRange = "24h";
        try {
            await loadDashboard();
        }
        catch (error) {
            showError(error.message ?? String(error));
        }
    });

    rangeWeekButton.addEventListener("click", async () => {
        selectedRange = "week";
        try {
            await loadDashboard();
        }
        catch (error) {
            showError(error.message ?? String(error));
        }
    });

    rangeMonthButton.addEventListener("click", async () => {
        selectedRange = "month";
        try {
            await loadDashboard();
        }
        catch (error) {
            showError(error.message ?? String(error));
        }
    });

    graphCenterTimeInput.addEventListener("change", async () => {
        try {
            await loadDashboard();
        }
        catch (error) {
            showError(error.message ?? String(error));
        }
    });

    function formatDateTime(epochSeconds) {
        return new Date(epochSeconds * 1000).toLocaleString();
    }

    function updateBrowserCurrentTime() {
        if (browserCurrentTimeElement) {
            browserCurrentTimeElement.textContent = new Date().toLocaleString();
        }
    }

    async function updateServerCurrentTime() {
        const result = await getJson("/api/admin/user");

        console.log("Admin user result", result);

        if (!result.ok || !result.body || result.body.serverTime === undefined) {
            if (serverCurrentTimeElement) {
                serverCurrentTimeElement.textContent = "Unavailable";
            }
            return;
        }

        if (serverCurrentTimeElement) {
            serverCurrentTimeElement.textContent = formatDateTime(result.body.serverTime);
        }
    }

    setInterval(() => {
        if (!dashboard.classList.contains("hidden")) {
            updateBrowserCurrentTime();
        }
    }, 1000);

    restoreSession().catch((error) => {
        console.error(error);
        setLoggedOut();
    });
})();