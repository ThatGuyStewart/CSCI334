#include "Database.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <exception>
#include <iostream>
#include <memory>
#include <new>
#include <random>
#include <stdexcept>

namespace
{
	long long toEpochSeconds(std::chrono::system_clock::time_point value)
	{
		return std::chrono::duration_cast<std::chrono::seconds>(value.time_since_epoch()).count();
	}

	std::chrono::system_clock::time_point fromEpochSeconds(double value)
	{
		return std::chrono::system_clock::time_point(
			std::chrono::milliseconds(static_cast<long long>(value * 1000.0)));
	}

	bool isHolidaySeasonMonth(int month)
	{
		return month >= 11 || month <= 2;
	}
}

Database::Database(std::string host, std::string port, std::string dbname, std::string user, std::string password)
	: m_connection(nullptr)
{
	if (!connect(host, port, dbname, user, password))
	{
		throw std::runtime_error("Database connection failed.");
	}

	if (!createSchema())
	{
		throw std::runtime_error("Database schema creation failed.");
	}

	refreshBookingStatuses();
	loadAccounts();

	// Simulation will handle loading the car park data, so we initialize it with an empty car park here.
	// This allows us to avoid potential issues with loading the car park data before the database is populated.
	// un-comment the line below if not running the simulation, but make sure to comment it out again when running the simulation.
	
	//loadCarPark();
}

bool Database::connect(std::string& host, std::string& port, std::string& dbname, std::string& user, std::string& password)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	try
	{
		m_connection = std::make_unique<pqxx::connection>("host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + password);
		std::cout << "Database connection established." << std::endl;
		return m_connection->is_open();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Database connection failed: " << e.what() << std::endl;
		m_connection.reset();
		return false;
	}
}

bool Database::isConnected() const
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	return (m_connection != nullptr && m_connection->is_open());
}

std::vector<std::pair<int, int>> Database::predictAvailableNormal(std::time_t futureTime, int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return {};

	try
	{
		const std::tm futureTm = *std::localtime(&futureTime);

		const int targetDayOfWeek = futureTm.tm_wday;
		const int targetQuarterHour = ((futureTm.tm_hour * 60) + futureTm.tm_min) / 15;
		const bool futureIsHolidaySeason = isHolidaySeasonMonth(futureTm.tm_mon + 1);

		pqxx::read_transaction tx(*m_connection);

		pqxx::result result = (lotId == 0)
			? (futureIsHolidaySeason
				? tx.exec_params(
					"SELECT lot_id, CAST(ROUND(AVG(available_normal_spaces)) AS INT) AS predicted_available_normal_spaces "
					"FROM availability "
					"WHERE EXTRACT(DOW FROM snapshot_time) = $1 "
					"  AND ((EXTRACT(HOUR FROM snapshot_time)::int * 60 + EXTRACT(MINUTE FROM snapshot_time)::int) / 15) = $2 "
					"GROUP BY lot_id "
					"ORDER BY lot_id",
					targetDayOfWeek,
					targetQuarterHour)
				: tx.exec_params(
					"SELECT lot_id, CAST(ROUND(AVG(available_normal_spaces)) AS INT) AS predicted_available_normal_spaces "
					"FROM availability "
					"WHERE EXTRACT(DOW FROM snapshot_time) = $1 "
					"  AND ((EXTRACT(HOUR FROM snapshot_time)::int * 60 + EXTRACT(MINUTE FROM snapshot_time)::int) / 15) = $2 "
					"  AND EXTRACT(MONTH FROM snapshot_time) NOT IN (11, 12, 1, 2) "
					"GROUP BY lot_id "
					"ORDER BY lot_id",
					targetDayOfWeek,
					targetQuarterHour))
			: (futureIsHolidaySeason
				? tx.exec_params(
					"SELECT lot_id, CAST(ROUND(AVG(available_normal_spaces)) AS INT) AS predicted_available_normal_spaces "
					"FROM availability "
					"WHERE lot_id = $1 "
					"  AND EXTRACT(DOW FROM snapshot_time) = $2 "
					"  AND ((EXTRACT(HOUR FROM snapshot_time)::int * 60 + EXTRACT(MINUTE FROM snapshot_time)::int) / 15) = $3 "
					"GROUP BY lot_id",
					lotId,
					targetDayOfWeek,
					targetQuarterHour)
				: tx.exec_params(
					"SELECT lot_id, CAST(ROUND(AVG(available_normal_spaces)) AS INT) AS predicted_available_normal_spaces "
					"FROM availability "
					"WHERE lot_id = $1 "
					"  AND EXTRACT(DOW FROM snapshot_time) = $2 "
					"  AND ((EXTRACT(HOUR FROM snapshot_time)::int * 60 + EXTRACT(MINUTE FROM snapshot_time)::int) / 15) = $3 "
					"  AND EXTRACT(MONTH FROM snapshot_time) NOT IN (11, 12, 1, 2) "
					"GROUP BY lot_id ",
					lotId,
					targetDayOfWeek,
					targetQuarterHour));

		std::vector<std::pair<int, int>> predictions;
		predictions.reserve(result.size());

		for (const auto& row : result)
		{
			predictions.emplace_back(
				row["lot_id"].as<int>(),
				row["predicted_available_normal_spaces"].as<int>());
		}

		return predictions;
	}
	catch (const std::exception& e)
	{
		std::cout << "Predict available normal spaces failed: " << e.what() << std::endl;
		return {};
	}
}

std::vector<std::pair<int, int>> Database::predictAvailableDisabled(std::time_t futureTime, int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return {};

	try
	{
		const std::tm futureTm = *std::localtime(&futureTime);

		const int targetDayOfWeek = futureTm.tm_wday;
		const int targetQuarterHour = ((futureTm.tm_hour * 60) + futureTm.tm_min) / 15;
		const bool futureIsHolidaySeason = isHolidaySeasonMonth(futureTm.tm_mon + 1);

		pqxx::read_transaction tx(*m_connection);

		pqxx::result result = (lotId == 0)
			? (futureIsHolidaySeason
				? tx.exec_params(
					"SELECT lot_id, CAST(ROUND(AVG(available_disabled_spaces)) AS INT) AS predicted_available_disabled_spaces "
					"FROM availability "
					"WHERE EXTRACT(DOW FROM snapshot_time) = $1 "
					"  AND ((EXTRACT(HOUR FROM snapshot_time)::int * 60 + EXTRACT(MINUTE FROM snapshot_time)::int) / 15) = $2 "
					"GROUP BY lot_id "
					"ORDER BY lot_id",
					targetDayOfWeek,
					targetQuarterHour)
				: tx.exec_params(
					"SELECT lot_id, CAST(ROUND(AVG(available_disabled_spaces)) AS INT) AS predicted_available_disabled_spaces "
					"FROM availability "
					"WHERE EXTRACT(DOW FROM snapshot_time) = $1 "
					"  AND ((EXTRACT(HOUR FROM snapshot_time)::int * 60 + EXTRACT(MINUTE FROM snapshot_time)::int) / 15) = $2 "
					"  AND EXTRACT(MONTH FROM snapshot_time) NOT IN (11, 12, 1, 2) "
					"GROUP BY lot_id "
					"ORDER BY lot_id",
					targetDayOfWeek,
					targetQuarterHour))
			: (futureIsHolidaySeason
				? tx.exec_params(
					"SELECT lot_id, CAST(ROUND(AVG(available_disabled_spaces)) AS INT) AS predicted_available_disabled_spaces "
					"FROM availability "
					"WHERE lot_id = $1 "
					"  AND EXTRACT(DOW FROM snapshot_time) = $2 "
					"  AND ((EXTRACT(HOUR FROM snapshot_time)::int * 60 + EXTRACT(MINUTE FROM snapshot_time)::int) / 15) = $3 "
					"GROUP BY lot_id",
					lotId,
					targetDayOfWeek,
					targetQuarterHour)
				: tx.exec_params(
					"SELECT lot_id, CAST(ROUND(AVG(available_disabled_spaces)) AS INT) AS predicted_available_disabled_spaces "
					"FROM availability "
					"WHERE lot_id = $1 "
					"  AND EXTRACT(DOW FROM snapshot_time) = $2 "
					"  AND ((EXTRACT(HOUR FROM snapshot_time)::int * 60 + EXTRACT(MINUTE FROM snapshot_time)::int) / 15) = $3 "
					"  AND EXTRACT(MONTH FROM snapshot_time) NOT IN (11, 12, 1, 2) "
					"GROUP BY lot_id",
					lotId,
					targetDayOfWeek,
					targetQuarterHour));

		std::vector<std::pair<int, int>> predictions;
		predictions.reserve(result.size());

		for (const auto& row : result)
		{
			predictions.emplace_back(
				row["lot_id"].as<int>(),
				row["predicted_available_disabled_spaces"].as<int>());
		}

		return predictions;
	}
	catch (const std::exception& e)
	{
		std::cout << "Predict available disabled spaces failed: " << e.what() << std::endl;
		return {};
	}
}

std::vector<std::pair<int, int>> Database::predictAvailableReserved(std::time_t futureTime, int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return {};

	try
	{
		refreshBookingStatuses();

		pqxx::read_transaction tx(*m_connection);

		pqxx::result result = (lotId == 0)
			? tx.exec_params(
				"WITH capacities AS ( "
				"    SELECT lot_id, "
				"           SUM(CASE WHEN type = 'reserved' THEN 1 ELSE 0 END)::int AS reserved_capacity "
				"    FROM spaces "
				"    GROUP BY lot_id "
				") "
				"SELECT c.lot_id, "
				"       GREATEST(0, c.reserved_capacity - COUNT(b.*)::int) AS predicted_available_reserved_spaces "
				"FROM capacities c "
				"LEFT JOIN bookings b "
				"  ON b.lot_id = c.lot_id "
				" AND b.status = 'Active' "
				" AND b.start_time <= to_timestamp($1) "
				" AND b.end_time > to_timestamp($1) "
				"GROUP BY c.lot_id, c.reserved_capacity "
				"ORDER BY c.lot_id",
				static_cast<long long>(futureTime))
			: tx.exec_params(
				"WITH capacities AS ( "
				"    SELECT lot_id, "
				"           SUM(CASE WHEN type = 'reserved' THEN 1 ELSE 0 END)::int AS reserved_capacity "
				"    FROM spaces "
				"    WHERE lot_id = $1 "
				"    GROUP BY lot_id "
				") "
				"SELECT c.lot_id, "
				"       GREATEST(0, c.reserved_capacity - COUNT(b.*)::int) AS predicted_available_reserved_spaces "
				"FROM capacities c "
				"LEFT JOIN bookings b "
				"  ON b.lot_id = c.lot_id "
				" AND b.status = 'Active' "
				" AND b.start_time <= to_timestamp($2) "
				" AND b.end_time > to_timestamp($2) "
				"GROUP BY c.lot_id, c.reserved_capacity",
				lotId,
				static_cast<long long>(futureTime));

		std::vector<std::pair<int, int>> predictions;
		predictions.reserve(result.size());

		for (const auto& row : result)
		{
			predictions.emplace_back(
				row["lot_id"].as<int>(),
				row["predicted_available_reserved_spaces"].as<int>());
		}

		return predictions;
	}
	catch (const std::exception& e)
	{
		std::cout << "Predict available reserved spaces failed: " << e.what() << std::endl;
		return {};
	}
}

std::vector<TempBooking> Database::getUpcomingBookings(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	std::vector<TempBooking> bookings;
	if (!isConnected()) return bookings;

	try
	{
		refreshBookingStatuses();

		pqxx::read_transaction tx(*m_connection);
		pqxx::result result = tx.exec_params(
			"SELECT lot_id, email, registration, "
			"EXTRACT(EPOCH FROM start_time) AS start_epoch, "
			"EXTRACT(EPOCH FROM end_time) AS end_epoch "
			"FROM bookings "
			"WHERE email = $1 AND status = 'Active' AND end_time > NOW() "
			"ORDER BY start_time",
			email);

		bookings.reserve(result.size());
		for (const auto& row : result)
		{
			bookings.push_back(
				{
					row["lot_id"].as<int>(),
					row["email"].as<std::string>(),
					row["registration"].as<std::string>(),
					fromEpochSeconds(row["start_epoch"].as<double>()),
					fromEpochSeconds(row["end_epoch"].as<double>())
				});
		}

		return bookings;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get upcoming bookings failed: " << e.what() << std::endl;
		return {};
	}
}

bool Database::loadAccounts()
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	if (!isConnected()) return false;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		pqxx::result result = tx.exec(
			"SELECT email, password, is_admin FROM accounts ORDER BY email");

		std::unordered_map<std::string, std::unique_ptr<Account>> accounts;
		accounts.reserve(result.size());

		for (const auto& row : result)
		{
			const std::string email = row["email"].as<std::string>();
			const std::string password = row["password"].as<std::string>();
			const bool isAdmin = row["is_admin"].as<bool>();

			if (isAdmin)
			{
				accounts[email] = std::make_unique<AdminAccount>(email, password);
			}
			else
			{
				accounts[email] = std::make_unique<UserAccount>(email, password);
			}
		}

		m_accounts = std::move(accounts);
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Load accounts failed: " << e.what() << std::endl;
		return false;
	}
}
std::unique_ptr<CarPark> Database::loadCarPark()
{
	std::cout << "Loading data from database..." << std::endl;
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	if (!isConnected())
	{
		throw std::runtime_error("Database is not connected.");
	}

	refreshBookingStatuses();
	return std::make_unique<CarPark>(loadLots());
}

std::unordered_map<int, std::unique_ptr<Lot>> Database::loadLots()
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	std::unordered_map<int, std::unique_ptr<Lot>> lots;
	if (!isConnected()) return lots;

	try
	{
		pqxx::result result;
		{
			pqxx::read_transaction tx(*m_connection);
			result = tx.exec("SELECT lot_id FROM lots");
		}

		if (result.empty()) throw std::runtime_error("No car park data found");

		for (const auto& row : result)
		{
			int lotId = row["lot_id"].as<int>();
			auto normalSpaces = loadNormalSpaces(lotId);
			auto disabledSpaces = loadDisabledSpaces(lotId);
			auto reservedSpaces = loadReservedSpaces(lotId);

			auto lot = std::make_unique<Lot>(
				lotId,
				std::move(normalSpaces),
				std::move(disabledSpaces),
				std::move(reservedSpaces));

			const auto bookings = loadBookings(lotId);
			for (const auto& booking : bookings)
			{
				lot->createBooking(
					booking.m_email,
					booking.m_registration,
					booking.m_start,
					booking.m_end);
			}

			lots[lotId] = std::move(lot);
		}

		return lots;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get car park data failed: " << e.what() << std::endl;
		throw;
	}
}

std::unordered_map<int, std::unique_ptr<Space>> Database::loadNormalSpaces(int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	std::unordered_map<int, std::unique_ptr<Space>> spaces;
	if (!isConnected()) return spaces;

	try
	{
		pqxx::read_transaction tx(*m_connection);

		pqxx::result result = tx.exec_params(
			"SELECT space_id FROM spaces WHERE lot_id = $1 AND type = 'normal' ORDER BY space_id",
			lotId);

		for (const auto& row : result)
		{
			const int spaceId = row["space_id"].as<int>();
			spaces[spaceId] = std::make_unique<Space>(lotId, spaceId);
		}

		return spaces;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get normal spaces failed: " << e.what() << std::endl;
		throw;
	}
}

std::unordered_map<int, std::unique_ptr<DisabledSpace>> Database::loadDisabledSpaces(int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	std::unordered_map<int, std::unique_ptr<DisabledSpace>> spaces;
	if (!isConnected()) return spaces;

	try
	{
		pqxx::read_transaction tx(*m_connection);

		pqxx::result result = tx.exec_params(
			"SELECT space_id FROM spaces WHERE lot_id = $1 AND type = 'disabled' ORDER BY space_id",
			lotId);

		for (const auto& row : result)
		{
			const int spaceId = row["space_id"].as<int>();
			spaces[spaceId] = std::make_unique<DisabledSpace>(lotId, spaceId);
		}

		return spaces;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get disabled spaces failed: " << e.what() << std::endl;
		throw;
	}
}

std::unordered_map<int, std::unique_ptr<ReservedSpace>> Database::loadReservedSpaces(int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	std::unordered_map<int, std::unique_ptr<ReservedSpace>> spaces;
	if (!isConnected()) return spaces;

	try
	{
		pqxx::read_transaction tx(*m_connection);

		pqxx::result result = tx.exec_params(
			"SELECT space_id FROM spaces WHERE lot_id = $1 AND type = 'reserved'",
			lotId);

		if (result.empty()) throw std::runtime_error(
			"No reserved spaces found for lot " + std::to_string(lotId));

		for (const auto& row : result)
		{
			const int spaceId = row["space_id"].as<int>();
			spaces[spaceId] = std::make_unique<ReservedSpace>(lotId, spaceId);
		}

		return spaces;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get reserved spaces failed: " << e.what() << std::endl;
		throw;
	}
}

std::vector<TempBooking> Database::loadBookings(int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	std::vector<TempBooking> bookings;
	if (!isConnected()) return bookings;

	try
	{
		pqxx::read_transaction tx(*m_connection);

		pqxx::result result = tx.exec_params(
			"SELECT email, registration, "
			"EXTRACT(EPOCH FROM start_time) AS start_epoch, "
			"EXTRACT(EPOCH FROM end_time) AS end_epoch "
			"FROM bookings "
			"WHERE lot_id = $1 AND status = 'Active' AND end_time > NOW()",
			lotId);

		bookings.reserve(result.size());
		for (const auto& row : result)
		{
			bookings.push_back(
				{
					lotId,
					row["email"].as<std::string>(),
					row["registration"].as<std::string>(),
					fromEpochSeconds(row["start_epoch"].as<double>()),
					fromEpochSeconds(row["end_epoch"].as<double>())
				});
		}

		return bookings;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get bookings failed: " << e.what() << std::endl;
		return bookings;
	}
}

bool Database::createAccount(const std::string& email, const std::string& password)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return false;
	if (accountExists(email)) return false;
	try
	{
		pqxx::work tx(*m_connection);

		tx.exec_params(
			"INSERT INTO accounts (email, password) VALUES ($1, $2)",
			email,
			password);

		tx.commit();
		m_accounts[email] = std::make_unique<UserAccount>(email, password);
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Create account failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::validateAccount(const std::string& email, const std::string& password)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	const auto it = m_accounts.find(email);
	if (it == m_accounts.end() || it->second == nullptr)
	{
		return false;
	}

	return it->second->validatePassword(password);
}

bool Database::accountExists(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	return m_accounts.find(email) != m_accounts.end();
}

bool Database::insertBooking(
	const std::string& email,
	const std::string& registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end,
	int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	if (!isConnected()) return false;
	if (registration.empty()) return false;
	if (start >= end) return false;
	if ((end - start) > std::chrono::hours(6)) return false;
	if (lotId <= 0) return false;
	if (!accountExists(email)) return false;

	try
	{
		refreshBookingStatuses();

		pqxx::work tx(*m_connection);

		const pqxx::result activeCountResult = tx.exec_params(
			"SELECT COUNT(*) AS active_count "
			"FROM bookings "
			"WHERE email = $1 AND status = 'Active'",
			email);

		if (!activeCountResult.empty() && activeCountResult[0]["active_count"].as<int>() >= 5)
		{
			return false;
		}

		const pqxx::result overlapResult = tx.exec_params(
			"SELECT 1 "
			"FROM bookings "
			"WHERE email = $1 "
			"  AND status = 'Active' "
			"  AND start_time < to_timestamp($2) "
			"  AND end_time > to_timestamp($3) "
			"LIMIT 1",
			email,
			toEpochSeconds(end),
			toEpochSeconds(start));

		if (!overlapResult.empty())
		{
			return false;
		}

		tx.exec_params(
			"INSERT INTO bookings (lot_id, email, registration, start_time, end_time, status) "
			"VALUES ($1, $2, $3, to_timestamp($4), to_timestamp($5), 'Active')",
			lotId,
			email,
			registration,
			toEpochSeconds(start),
			toEpochSeconds(end));

		tx.commit();
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Insert booking failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::cancelBookingRecord(
	const std::string& email,
	const std::string& registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end,
	int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	if (!isConnected()) return false;
	if (lotId <= 0) return false;
	if (registration.empty()) return false;
	if (start >= end) return false;

	try
	{
		refreshBookingStatuses();

		pqxx::work tx(*m_connection);
		pqxx::result result = tx.exec_params(
			"UPDATE bookings "
			"SET status = 'Cancelled' "
			"WHERE lot_id = $1 "
			"  AND email = $2 "
			"  AND registration = $3 "
			"  AND EXTRACT(EPOCH FROM start_time)::bigint = $4 "
			"  AND EXTRACT(EPOCH FROM end_time)::bigint = $5 "
			"  AND status = 'Active' "
			"RETURNING lot_id",
			lotId,
			email,
			registration,
			toEpochSeconds(start),
			toEpochSeconds(end));

		if (result.empty())
		{
			return false;
		}

		tx.commit();
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Cancel booking record failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::updateBookingRecord(
	const std::string& email,
	const std::string& originalRegistration,
	std::chrono::system_clock::time_point originalStart,
	std::chrono::system_clock::time_point originalEnd,
	int originalLotId,
	const std::string& newRegistration,
	std::chrono::system_clock::time_point newStart,
	std::chrono::system_clock::time_point newEnd,
	int newLotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	if (!isConnected()) return false;
	if (originalLotId <= 0 || newLotId <= 0) return false;
	if (originalRegistration.empty() || newRegistration.empty()) return false;
	if (originalStart >= originalEnd || newStart >= newEnd) return false;
	if ((newEnd - newStart) > std::chrono::hours(6)) return false;

	try
	{
		refreshBookingStatuses();

		pqxx::work tx(*m_connection);

		pqxx::result updateResult = tx.exec_params(
			"UPDATE bookings "
			"SET status = 'Cancelled' "
			"WHERE lot_id = $1 "
			"  AND email = $2 "
			"  AND registration = $3 "
			"  AND EXTRACT(EPOCH FROM start_time)::bigint = $4 "
			"  AND EXTRACT(EPOCH FROM end_time)::bigint = $5 "
			"  AND status = 'Active' "
			"RETURNING lot_id",
			originalLotId,
			email,
			originalRegistration,
			toEpochSeconds(originalStart),
			toEpochSeconds(originalEnd));

		if (updateResult.empty())
		{
			return false;
		}

		const pqxx::result activeCountResult = tx.exec_params(
			"SELECT COUNT(*) AS active_count "
			"FROM bookings "
			"WHERE email = $1 AND status = 'Active'",
			email);

		if (!activeCountResult.empty() && activeCountResult[0]["active_count"].as<int>() >= 5)
		{
			tx.abort();
			return false;
		}

		const pqxx::result overlapResult = tx.exec_params(
			"SELECT 1 "
			"FROM bookings "
			"WHERE email = $1 "
			"  AND status = 'Active' "
			"  AND start_time < to_timestamp($2) "
			"  AND end_time > to_timestamp($3) "
			"LIMIT 1",
			email,
			toEpochSeconds(newEnd),
			toEpochSeconds(newStart));

		if (!overlapResult.empty())
		{
			tx.abort();
			return false;
		}

		tx.exec_params(
			"INSERT INTO bookings (lot_id, email, registration, start_time, end_time, status) "
			"VALUES ($1, $2, $3, to_timestamp($4), to_timestamp($5), 'Active')",
			newLotId,
			email,
			newRegistration,
			toEpochSeconds(newStart),
			toEpochSeconds(newEnd));

		tx.commit();
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Update booking record failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::createSchema()
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	std::cout << "Creating database schema..." << std::endl;
	try
	{
		pqxx::work tx(*m_connection);

		tx.exec(R"(
			CREATE TABLE IF NOT EXISTS accounts
			(
				email TEXT PRIMARY KEY,
				password TEXT NOT NULL,
				is_admin BOOLEAN NOT NULL DEFAULT FALSE
			);

			ALTER TABLE accounts
			ADD COLUMN IF NOT EXISTS is_admin BOOLEAN NOT NULL DEFAULT FALSE;

			CREATE TABLE IF NOT EXISTS lots
			(
				lot_id SERIAL PRIMARY KEY
			);

			CREATE TABLE IF NOT EXISTS spaces
			(
				lot_id INT NOT NULL,
				space_id INT NOT NULL,
				type TEXT NOT NULL CHECK (type IN ('normal', 'disabled', 'reserved')),
				PRIMARY KEY (lot_id, space_id),
				FOREIGN KEY (lot_id) REFERENCES lots(lot_id) ON DELETE CASCADE
			);

			CREATE TABLE IF NOT EXISTS bookings
			(
				id SERIAL PRIMARY KEY,
				lot_id INT NOT NULL,
				email TEXT NOT NULL,
				registration TEXT NOT NULL,
				start_time TIMESTAMPTZ NOT NULL,
				end_time TIMESTAMPTZ NOT NULL,
				status TEXT NOT NULL DEFAULT 'Active',
				created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
				updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
				FOREIGN KEY (email) REFERENCES accounts(email) ON DELETE CASCADE,
				FOREIGN KEY (lot_id) REFERENCES lots(lot_id) ON DELETE CASCADE
			);

			ALTER TABLE bookings
			ADD COLUMN IF NOT EXISTS status TEXT NOT NULL DEFAULT 'Active';

			ALTER TABLE bookings
			ADD COLUMN IF NOT EXISTS created_at TIMESTAMPTZ NOT NULL DEFAULT NOW();

			ALTER TABLE bookings
			ADD COLUMN IF NOT EXISTS updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW();

			ALTER TABLE bookings
			DROP CONSTRAINT IF EXISTS bookings_lot_id_space_id_fkey;

			ALTER TABLE bookings
			DROP COLUMN IF EXISTS space_id;

			ALTER TABLE bookings
			ALTER COLUMN start_time TYPE TIMESTAMPTZ
			USING start_time AT TIME ZONE current_setting('TIMEZONE');

			ALTER TABLE bookings
			ALTER COLUMN end_time TYPE TIMESTAMPTZ
			USING end_time AT TIME ZONE current_setting('TIMEZONE');

			ALTER TABLE bookings
			DROP CONSTRAINT IF EXISTS chk_bookings_status;

			ALTER TABLE bookings
			ADD CONSTRAINT chk_bookings_status
			CHECK (status IN ('Active', 'Cancelled', 'Expired'));

			ALTER TABLE bookings
			DROP CONSTRAINT IF EXISTS chk_bookings_time_range;

			ALTER TABLE bookings
			ADD CONSTRAINT chk_bookings_time_range
			CHECK (end_time > start_time);

			UPDATE bookings
			SET status = 'Expired',
				updated_at = NOW()
			WHERE status = 'Active' AND end_time <= NOW();

			CREATE INDEX IF NOT EXISTS idx_bookings_lot_status_start_end
			ON bookings(lot_id, status, start_time, end_time);

			ALTER TABLE bookings
			DROP CONSTRAINT IF EXISTS chk_bookings_max_duration;

			ALTER TABLE bookings
			ADD CONSTRAINT chk_bookings_max_duration
			CHECK (end_time <= start_time + INTERVAL '6 hours');

			CREATE TABLE IF NOT EXISTS availability
			(
				lot_id INT NOT NULL,
				snapshot_time TIMESTAMPTZ NOT NULL,
				available_normal_spaces INT NOT NULL,
				available_disabled_spaces INT NOT NULL,
				available_reserved_spaces INT NOT NULL DEFAULT 0,
				updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
				PRIMARY KEY (lot_id, snapshot_time),
				FOREIGN KEY (lot_id) REFERENCES lots(lot_id) ON DELETE CASCADE
			);
			ALTER TABLE availability
			ADD COLUMN IF NOT EXISTS available_reserved_spaces INT NOT NULL DEFAULT 0;			

			ALTER TABLE availability
			ALTER COLUMN updated_at TYPE TIMESTAMPTZ
			USING updated_at AT TIME ZONE current_setting('TIMEZONE');

			ALTER TABLE availability
			ALTER COLUMN snapshot_time TYPE TIMESTAMPTZ
			USING snapshot_time AT TIME ZONE current_setting('TIMEZONE');

			ALTER TABLE availability
			ADD COLUMN IF NOT EXISTS reserved_occupied_spaces INT NOT NULL DEFAULT 0;

			CREATE INDEX IF NOT EXISTS idx_availability_snapshot_time
			ON availability(snapshot_time);
		)");

		tx.commit();
		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Database schema creation failed: " << e.what() << std::endl;
		return false;
	}
}

void Database::refreshBookingStatuses()
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return;

	try
	{
		pqxx::work tx(*m_connection);
		tx.exec(
			"UPDATE bookings "
			"SET status = 'Expired' "
			"WHERE status = 'Active' AND end_time <= NOW()");
		tx.commit();
	}
	catch (const std::exception& e)
	{
		std::cout << "Refresh booking statuses failed: " << e.what() << std::endl;
	}
}

std::unordered_map<int, std::unordered_map<std::time_t, std::vector<int>>> Database::getLotActivity()
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	std::unordered_map<int, std::unordered_map<std::time_t, std::vector<int>>> activity;
	if (!isConnected()) return activity;

	try
	{
		pqxx::read_transaction tx(*m_connection);

		pqxx::result result = tx.exec(
			"WITH capacities AS ( "
			"    SELECT "
			"        lot_id, "
			"        SUM(CASE WHEN type = 'normal' THEN 1 ELSE 0 END)::int AS normal_capacity, "
			"        SUM(CASE WHEN type = 'disabled' THEN 1 ELSE 0 END)::int AS disabled_capacity, "
			"        SUM(CASE WHEN type = 'reserved' THEN 1 ELSE 0 END)::int AS reserved_capacity "
			"    FROM spaces "
			"    GROUP BY lot_id "
			") "
			"SELECT "
			"    a.lot_id, "
			"    EXTRACT(EPOCH FROM a.snapshot_time)::bigint AS snapshot_epoch, "
			"    GREATEST(0, c.normal_capacity - a.available_normal_spaces) AS normal_occupied, "
			"    GREATEST(0, c.disabled_capacity - a.available_disabled_spaces) AS disabled_occupied, "
			"    GREATEST(0, c.reserved_capacity - a.available_reserved_spaces) AS reserved_occupied "
			"FROM availability a "
			"INNER JOIN capacities c ON c.lot_id = a.lot_id "
			"ORDER BY a.lot_id, a.snapshot_time");

		for (const auto& row : result)
		{
			const int currentLotId = row["lot_id"].as<int>();
			const std::time_t snapshotTime = static_cast<std::time_t>(row["snapshot_epoch"].as<long long>());
			const int normalOccupied = row["normal_occupied"].as<int>();
			const int disabledOccupied = row["disabled_occupied"].as<int>();
			const int reservedOccupied = row["reserved_occupied"].as<int>();

			activity[currentLotId][snapshotTime] =
			{
				normalOccupied,
				disabledOccupied,
				reservedOccupied
			};
		}

		return activity;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get lot activity failed: " << e.what() << std::endl;
		return {};
	}
}

bool Database::saveAvailabilitySnapshot(
	std::time_t snapshotTime,
	const std::unordered_map<int, std::vector<int>>& availabilityByLot)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	if (!isConnected()) return false;

	try
	{
		pqxx::work tx(*m_connection);

		for (const auto& [lotId, values] : availabilityByLot)
		{
			if (values.size() < 2) continue;

			const int availableNormalSpaces = values[0];
			const int availableDisabledSpaces = values[1];
			const int availableReservedSpaces = values.size() > 2 ? values[2] : 0;

			tx.exec_params(
				"INSERT INTO availability "
				"(lot_id, snapshot_time, available_normal_spaces, available_disabled_spaces, available_reserved_spaces, updated_at) "
				"VALUES ($1, to_timestamp($2), $3, $4, $5, NOW()) "
				"ON CONFLICT (lot_id, snapshot_time) "
				"DO UPDATE SET "
				"available_normal_spaces = EXCLUDED.available_normal_spaces, "
				"available_disabled_spaces = EXCLUDED.available_disabled_spaces, "
				"available_reserved_spaces = EXCLUDED.available_reserved_spaces, "
				"updated_at = NOW()",
				lotId,
				static_cast<long long>(snapshotTime),
				availableNormalSpaces,
				availableDisabledSpaces,
				availableReservedSpaces);
		}

		tx.commit();
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Save availability snapshot failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::isAdminAccount(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	const auto it = m_accounts.find(email);
	return it != m_accounts.end() && it->second != nullptr && it->second->isAdmin();
}

std::vector<TempBooking> Database::getCurrentBookingsForLot(int lotId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	std::vector<TempBooking> bookings;
	if (!isConnected()) return bookings;
	if (lotId <= 0) return bookings;

	try
	{
		refreshBookingStatuses();

		pqxx::read_transaction tx(*m_connection);
		pqxx::result result = tx.exec_params(
			"SELECT lot_id, email, registration, "
			"EXTRACT(EPOCH FROM start_time) AS start_epoch, "
			"EXTRACT(EPOCH FROM end_time) AS end_epoch "
			"FROM bookings "
			"WHERE lot_id = $1 "
			"  AND status = 'Active' "
			"  AND start_time <= NOW() "
			"  AND end_time > NOW() "
			"ORDER BY start_time, email, registration",
			lotId);

		bookings.reserve(result.size());
		for (const auto& row : result)
		{
			bookings.push_back(
				{
					row["lot_id"].as<int>(),
					row["email"].as<std::string>(),
					row["registration"].as<std::string>(),
					fromEpochSeconds(row["start_epoch"].as<double>()),
					fromEpochSeconds(row["end_epoch"].as<double>())
				});
		}

		return bookings;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get current bookings for lot failed: " << e.what() << std::endl;
		return {};
	}
}
