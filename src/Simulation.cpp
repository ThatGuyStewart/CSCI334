#include "Simulation.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <thread>

namespace
{
	bool isHolidaySeason(int month)
	{
		return month >= 11 || month < 3;
	}

	bool isHoliday(int month, int day)
	{
		static constexpr std::array<std::pair<int, int>, 4> holidays =
		{ {
			{1, 1},
			{4, 25},
			{12, 25},
			{12, 26}
		} };

		for (const auto& holiday : holidays)
		{
			if (holiday.first == month && holiday.second == day)
			{
				return true;
			}
		}

		return false;
	}

	int sampleTargetOccupied(int capacity, double minOccupancy, double maxOccupancy, std::mt19937& rng)
	{
		std::uniform_real_distribution<double> occupancyDist(minOccupancy, maxOccupancy);
		return static_cast<int>(std::round(capacity * occupancyDist(rng)));
	}

	int sampleHistoricalOccupiedDurationSlots(std::mt19937& rng)
	{
		static std::discrete_distribution<int> durationDist
		{
			1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
		};

		return 2 + durationDist(rng);
	}

	int countOccupiedSpaces(const std::vector<int>& remainingOccupiedSlots)
	{
		return static_cast<int>(std::count_if(
			remainingOccupiedSlots.begin(),
			remainingOccupiedSlots.end(),
			[](int remaining) { return remaining > 0; }));
	}

	void addHistoricalOccupancy(std::vector<int>& remainingOccupiedSlots, int targetOccupied, std::mt19937& rng)
	{
		const int currentOccupied = countOccupiedSpaces(remainingOccupiedSlots);
		if (currentOccupied >= targetOccupied) return;

		std::vector<int> availableIndices;
		availableIndices.reserve(remainingOccupiedSlots.size());

		for (int i = 0; i < static_cast<int>(remainingOccupiedSlots.size()); ++i)
		{
			if (remainingOccupiedSlots[i] == 0)
			{
				availableIndices.push_back(i);
			}
		}

		std::shuffle(availableIndices.begin(), availableIndices.end(), rng);

		const int toOccupy = std::min(targetOccupied - currentOccupied, static_cast<int>(availableIndices.size()));
		for (int i = 0; i < toOccupy; ++i)
		{
			remainingOccupiedSlots[availableIndices[i]] = sampleHistoricalOccupiedDurationSlots(rng);
		}
	}

	void trimHistoricalOccupancy(std::vector<int>& remainingOccupiedSlots, int targetOccupied, std::mt19937& rng)
	{
		const int currentOccupied = countOccupiedSpaces(remainingOccupiedSlots);
		if (currentOccupied <= targetOccupied) return;

		std::vector<int> occupiedIndices;
		occupiedIndices.reserve(remainingOccupiedSlots.size());

		for (int i = 0; i < static_cast<int>(remainingOccupiedSlots.size()); ++i)
		{
			if (remainingOccupiedSlots[i] > 0)
			{
				occupiedIndices.push_back(i);
			}
		}

		std::shuffle(occupiedIndices.begin(), occupiedIndices.end(), rng);

		const int toClear = std::min(
			currentOccupied - targetOccupied,
			static_cast<int>(occupiedIndices.size()));

		for (int i = 0; i < toClear; ++i)
		{
			remainingOccupiedSlots[occupiedIndices[i]] = 0;
		}
	}

   void advanceHistoricalOccupancyDurations(std::vector<int>& remainingOccupiedSlots)
	{
		for (int& remaining : remainingOccupiedSlots)
		{
			if (remaining > 0)
			{
				--remaining;
			}
		}		
	}
}
	
Simulation::Simulation(Server& server, CarParkService& service, Database& db)
	: m_server(server), m_service(service), m_db(db), m_rng(std::random_device{}())
{
}

void Simulation::run()
{
	seedDatabaseWithHistoricalData();
	m_threadControl.running.store(true);

	std::thread simulationThread(&Simulation::simulateParkingBehavior, this);
	m_server.start();

	m_threadControl.running.store(false);

	if (simulationThread.joinable())
	{
		simulationThread.join();
	}
}

void Simulation::stop()
{
	m_threadControl.running.store(false);
	m_threadControl.cv.notify_all();
}

OccupancyParameters Simulation::getOccupancyParameters(const std::tm& localTm) const
{
	const bool weekend = (localTm.tm_wday == 0 || localTm.tm_wday == 6);
	const bool holiday = isHoliday(localTm.tm_mon + 1, localTm.tm_mday);
	const bool holidaySeason = isHolidaySeason(localTm.tm_mon + 1);

	OccupancyParameters parameters{ 0.20, 0.40, 0.10, 0.20 };

	if (localTm.tm_hour < 6)
	{
		parameters.normalMin = 0.00;
		parameters.normalMax = 0.05;
		parameters.disabledMin = 0.00;
		parameters.disabledMax = 0.05;
	}
	else if (weekend || holiday || holidaySeason)
	{
		parameters.normalMin = 0.10;
		parameters.normalMax = 0.20;
		parameters.disabledMin = 0.10;
		parameters.disabledMax = 0.20;
	}
	else if (localTm.tm_hour == 8)
	{
		const double progress = localTm.tm_min / 60.0;
		parameters.normalMin = 0.20 + (0.60 * progress);
		parameters.normalMax = 0.30 + (0.70 * progress);
		parameters.disabledMin = 0.00 + (0.40 * progress);
		parameters.disabledMax = 0.30 + (0.60 * progress);
	}
	else if (localTm.tm_hour >= 9 && localTm.tm_hour < 16)
	{
		parameters.normalMin = 0.80;
		parameters.normalMax = 1.00;
		parameters.disabledMin = 0.40;
		parameters.disabledMax = 0.90;
	}
	else if (localTm.tm_hour == 16)
	{
		const double progress = localTm.tm_min / 60.0;
		parameters.normalMin = 0.80 + (-0.60 * progress);
		parameters.normalMax = 1.00 + (- 0.80 * progress);
		parameters.disabledMin = 0.80 + (-0.80 * progress);
		parameters.disabledMax = 0.90 + (-0.50 * progress);
	}
	else
	{
		parameters.normalMin = 0.10;
		parameters.normalMax = 0.25;
		parameters.disabledMin = 0.05;
		parameters.disabledMax = 0.15;
	}

	return parameters;
}

std::vector<Simulation::CandidateChange> Simulation::buildCandidateChanges(
	const std::vector<std::pair<int, std::vector<std::pair<int, bool>>>>& lots,
	double minOccupancy,
	double maxOccupancy)
{
	std::vector<CandidateChange> candidates;

	for (const auto& [currentLotId, spaces] : lots)
	{
		if (spaces.empty()) continue;

		int currentOccupied = 0;
		std::vector<int> availableSpaceIds;
		std::vector<int> occupiedSpaceIds;

		for (const auto& [currentSpaceId, available] : spaces)
		{
			if (available)
			{
				availableSpaceIds.push_back(currentSpaceId);
			}
			else
			{
				occupiedSpaceIds.push_back(currentSpaceId);
				++currentOccupied;
			}
		}

		int targetOccupied = sampleTargetOccupied(spaces.size(), minOccupancy, maxOccupancy, m_rng);
		const int difference = targetOccupied - currentOccupied;

		if (difference > 0)
		{
			for (int currentSpaceId : availableSpaceIds)
			{
				candidates.push_back({ currentLotId, currentSpaceId, true, difference });
			}
		}
		else if (difference < 0)
		{
			for (int currentSpaceId : occupiedSpaceIds)
			{
				candidates.push_back({ currentLotId, currentSpaceId, false, -difference });
			}
		}
	}

	return candidates;
}

void Simulation::simulateParkingBehavior()
{
	std::uniform_int_distribution<int> updateRnd(1, 5);

	while (m_threadControl.running.load())
	{
		std::unique_lock<std::mutex> stopLock(m_threadControl.mutex);
		m_threadControl.cv.wait_for(
			stopLock,
			std::chrono::seconds(1),
			[this]() { return !m_threadControl.running.load(); });

		if (!m_threadControl.running.load()) break;
		stopLock.unlock();

		if (updateRnd(m_rng) != 5) continue;

		std::time_t currentTime = std::time(nullptr);
		std::tm localTm{};
#if defined(_WIN32)
		localtime_s(&localTm, &currentTime);
#else
		localTm = *std::localtime(&currentTime);
#endif

		const OccupancyParameters parameters = getOccupancyParameters(localTm);
		auto candidates = buildCandidateChanges(
			m_service.getAvailableNormal(0),
			parameters.normalMin,
			parameters.normalMax);

		auto disabledCandidates = buildCandidateChanges(
			m_service.getAvailableDisabled(0),
			parameters.disabledMin,
			parameters.disabledMax);

		candidates.insert(candidates.end(), disabledCandidates.begin(), disabledCandidates.end());
		if (candidates.empty()) continue;

		std::shuffle(candidates.begin(), candidates.end(), m_rng);
		std::sort(
			candidates.begin(),
			candidates.end(),
			[](const CandidateChange& left, const CandidateChange& right)
			{
				return left.priority > right.priority;
			});

		const CandidateChange& selected = candidates.front();
		{
			std::lock_guard<std::recursive_mutex> lock(m_db.m_dbMutex);

			auto lotIt = m_service.m_carPark->m_lots.find(selected.lotId);
			if (lotIt == m_service.m_carPark->m_lots.end() || !lotIt->second)
			{
				continue;
			}

			Space& space = lotIt->second->getSpace(selected.spaceId);
			try
			{
				if (space.isOccupied() == selected.occupied)
				{
					continue;
				}
				if (!space.canBecomeAvailable(std::chrono::system_clock::now()))
				{
					continue;
				}

				space.setOccupied(selected.occupied);
			}
			catch (const std::exception& e)
			{
				std::cout << "Error checking space occupancy: " << e.what() << std::endl;
				continue;
			}
		}
	}
}

void Simulation::seedDatabaseWithHistoricalData()
{
	std::lock_guard<std::recursive_mutex> lock(m_db.m_dbMutex);
	std::cout << "Seeding database..." << std::endl;
	if (!m_db.isConnected())
	{
		throw std::runtime_error("Database is not connected.");
	}

	pqxx::work tx(*m_db.m_connection);

	tx.exec("TRUNCATE TABLE availability, bookings, spaces, lots, accounts RESTART IDENTITY CASCADE");

	const std::vector<std::string> emails =
	{
		"admin@example.com",
		"alice@example.com",
		"bob@example.com",
		"charlie@example.com",
		"diana@example.com",
		"eve@example.com",
		"frank@example.com",
		"grace@example.com",
		"henry@example.com",
		"isla@example.com"
	};

	for (const auto& email : emails)
	{
		tx.exec_params(
			"INSERT INTO accounts (email, password, is_admin) VALUES ($1, $2, $3)",
			email,
			"password123",
			email == "admin@example.com");
	}

	std::vector<int> lotIds;
	lotIds.reserve(3);

	for (int i = 0; i < 3; ++i)
	{
		pqxx::result res = tx.exec("INSERT INTO lots DEFAULT VALUES RETURNING lot_id");
		if (res.size() != 1)
		{
			throw std::runtime_error("Expected exactly one row from INSERT");
		}

		lotIds.push_back(res[0]["lot_id"].as<int>());
	}

	for (int lotId : lotIds)
	{
		for (int i = 1; i <= 80; ++i)
		{
			tx.exec_params(
				"INSERT INTO spaces (space_id, lot_id, type) VALUES ($1, $2, $3)",
				i,
				lotId,
				"normal");
		}

		for (int i = 81; i <= 90; ++i)
		{
			tx.exec_params(
				"INSERT INTO spaces (space_id, lot_id, type) VALUES ($1, $2, $3)",
				i,
				lotId,
				"disabled");
		}

		for (int i = 91; i <= 100; ++i)
		{
			tx.exec_params(
				"INSERT INTO spaces (space_id, lot_id, type) VALUES ($1, $2, $3)",
				i,
				lotId,
				"reserved");
		}
	}

	int bookingCounter = 0;
	const auto now = std::chrono::system_clock::now();
	const auto currentStartBase = std::chrono::system_clock::time_point(
		std::chrono::minutes(
			std::chrono::duration_cast<std::chrono::minutes>(now.time_since_epoch()).count()));

	for (int lotId : lotIds)
	{
		const int currentBookingsPerLot = 2;
		const int futureBookingsPerLot = 3;

		for (int i = 0; i < currentBookingsPerLot; ++i)
		{
			const std::string& email = emails[bookingCounter % emails.size()];
			const std::string registration = "LIVE-" + std::to_string(100 + bookingCounter);

			const auto startTime = currentStartBase - std::chrono::minutes(30 + (i * 15));
			const auto endTime = startTime + std::chrono::hours(2);

			const auto startEpoch = std::chrono::duration_cast<std::chrono::seconds>(startTime.time_since_epoch()).count();
			const auto endEpoch = std::chrono::duration_cast<std::chrono::seconds>(endTime.time_since_epoch()).count();

			tx.exec_params(
				"INSERT INTO bookings (lot_id, email, registration, start_time, end_time, status) "
				"VALUES ($1, $2, $3, to_timestamp($4), to_timestamp($5), 'Active')",
				lotId,
				email,
				registration,
				startEpoch,
				endEpoch);

			++bookingCounter;
		}

		for (int i = 0; i < futureBookingsPerLot; ++i)
		{
			const std::string& email = emails[bookingCounter % emails.size()];
			const std::string registration = "TEST-" + std::to_string(100 + bookingCounter);

			const auto rawStartTime = currentStartBase + std::chrono::hours(24 * (i + 1)) + std::chrono::hours(lotId);
			const auto startTime = std::chrono::system_clock::time_point(
				std::chrono::minutes(
					std::chrono::duration_cast<std::chrono::minutes>(rawStartTime.time_since_epoch()).count()));
			const auto endTime = startTime + std::chrono::hours(2);

			const auto startEpoch = std::chrono::duration_cast<std::chrono::seconds>(startTime.time_since_epoch()).count();
			const auto endEpoch = std::chrono::duration_cast<std::chrono::seconds>(endTime.time_since_epoch()).count();

			tx.exec_params(
				"INSERT INTO bookings (lot_id, email, registration, start_time, end_time, status) "
				"VALUES ($1, $2, $3, to_timestamp($4), to_timestamp($5), 'Active')",
				lotId,
				email,
				registration,
				startEpoch,
				endEpoch);

			++bookingCounter;
		}
	}

	std::unordered_map<int, std::vector<int>> normalRemainingByLot;
	std::unordered_map<int, std::vector<int>> disabledRemainingByLot;

	for (int lotId : lotIds)
	{
		normalRemainingByLot.emplace(lotId, std::vector<int>(80, 0));
		disabledRemainingByLot.emplace(lotId, std::vector<int>(10, 0));
	}

	for (int daysAgo = 84; daysAgo >= 1; --daysAgo)
	{
		for (int quarterHour = 0; quarterHour < 96; ++quarterHour)
		{
			const int minutesOfDay = quarterHour * 15;
			const int hour = minutesOfDay / 60;
			const int minute = minutesOfDay % 60;

			std::time_t currentTime = std::time(nullptr);
			std::tm localTm{};
#if defined(_WIN32)
			localtime_s(&localTm, &currentTime);
#else
			localTm = *std::localtime(&currentTime);
#endif

			localTm.tm_mday -= daysAgo;
			localTm.tm_hour = hour;
			localTm.tm_min = minute;
			localTm.tm_sec = 0;
			localTm.tm_isdst = -1;

			const std::time_t snapshotEpoch = std::mktime(&localTm);

#if defined(_WIN32)
			localtime_s(&localTm, &snapshotEpoch);
#else
			localTm = *std::localtime(&snapshotEpoch);
#endif

			const OccupancyParameters params = getOccupancyParameters(localTm);

			for (int lotId : lotIds)
			{
				auto& normalRemaining = normalRemainingByLot.at(lotId);
				auto& disabledRemaining = disabledRemainingByLot.at(lotId);

				const int normalCapacity = static_cast<int>(normalRemaining.size());
				const int disabledCapacity = static_cast<int>(disabledRemaining.size());

				const int targetOccupiedNormal = sampleTargetOccupied(normalCapacity, params.normalMin, params.normalMax, m_rng);
				const int targetOccupiedDisabled = sampleTargetOccupied(disabledCapacity, params.disabledMin, params.disabledMax, m_rng);

				trimHistoricalOccupancy(normalRemaining, targetOccupiedNormal, m_rng);
				trimHistoricalOccupancy(disabledRemaining, targetOccupiedDisabled, m_rng);

				addHistoricalOccupancy(normalRemaining, targetOccupiedNormal, m_rng);
				addHistoricalOccupancy(disabledRemaining, targetOccupiedDisabled, m_rng);

				const int occupiedNormal = countOccupiedSpaces(normalRemaining);
				const int occupiedDisabled = countOccupiedSpaces(disabledRemaining);

				pqxx::result reservedResult = tx.exec_params(
					"SELECT COUNT(*)::int AS reserved_occupied "
					"FROM bookings "
					"WHERE lot_id = $1 "
					"  AND status = 'Active' "
					"  AND start_time <= to_timestamp($2) "
					"  AND end_time > to_timestamp($2)",
					lotId,
					static_cast<long long>(snapshotEpoch));

				const int reservedOccupied = reservedResult.empty()
					? 0
					: reservedResult[0]["reserved_occupied"].as<int>();

				tx.exec_params(
					"INSERT INTO availability "
					"(lot_id, snapshot_time, available_normal_spaces, available_disabled_spaces, reserved_occupied_spaces, updated_at) "
					"VALUES ($1, to_timestamp($2), $3, $4, $5, NOW()) "
					"ON CONFLICT (lot_id, snapshot_time) "
					"DO UPDATE SET "
					"available_normal_spaces = EXCLUDED.available_normal_spaces, "
					"available_disabled_spaces = EXCLUDED.available_disabled_spaces, "
					"reserved_occupied_spaces = EXCLUDED.reserved_occupied_spaces, "
					"updated_at = NOW()",
					lotId,
					static_cast<long long>(snapshotEpoch),
					std::max(0, normalCapacity - occupiedNormal),
					std::max(0, disabledCapacity - occupiedDisabled),
					reservedOccupied);

                advanceHistoricalOccupancyDurations(normalRemaining);
				advanceHistoricalOccupancyDurations(disabledRemaining);
			}
		}
	}
	tx.exec_params(
		"UPDATE accounts "
		"SET is_admin = TRUE "
		"WHERE email = 'admin@example.com';");
	tx.commit();

	if (!m_db.loadAccounts())
	{
		throw std::runtime_error("Failed to reload accounts after seeding.");
	}

	m_service.loadCarPark();

	{
		std::time_t currentTime = std::time(nullptr);
		std::tm localTm{};

#ifdef _WIN32

		localtime_s(&localTm, &currentTime);
#else
		localTm = *std::localtime(&currentTime);
#endif
		const OccupancyParameters params = getOccupancyParameters(localTm);

		for (auto& [lotId, lot] : m_service.m_carPark->m_lots)
		{
			if (!lot) continue;

			std::vector<Space*> normalSpaces;
			normalSpaces.reserve(lot->m_normalSpaces.size());

			for (auto& [spaceId, space] : lot->m_normalSpaces)
			{
				if (space)
				{
					space->setOccupied(false);
					normalSpaces.push_back(space.get());
				}
			}

			std::vector<DisabledSpace*> disabledSpaces;
			disabledSpaces.reserve(lot->m_disabledSpaces.size());

			for (auto& [spaceId, space] : lot->m_disabledSpaces)
			{
				if (space)
				{
					space->setOccupied(false);
					disabledSpaces.push_back(space.get());
				}
			}

			const int targetOccupiedNormal = sampleTargetOccupied(
				static_cast<int>(normalSpaces.size()),
				params.normalMin,
				params.normalMax,
				m_rng);

			const int targetOccupiedDisabled = sampleTargetOccupied(
				static_cast<int>(disabledSpaces.size()),
				params.disabledMin,
				params.disabledMax,
				m_rng);

			std::shuffle(normalSpaces.begin(), normalSpaces.end(), m_rng);
			std::shuffle(disabledSpaces.begin(), disabledSpaces.end(), m_rng);

			for (int i = 0; i < targetOccupiedNormal && i < static_cast<int>(normalSpaces.size()); ++i)
			{
				normalSpaces[i]->setOccupied(true);
			}

			for (int i = 0; i < targetOccupiedDisabled && i < static_cast<int>(disabledSpaces.size()); ++i)
			{
				disabledSpaces[i]->setOccupied(true);
			}
		}
	}

}
