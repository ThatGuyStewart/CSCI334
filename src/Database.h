#pragma once
#include "ThreadControl.h"
#include "Account.h"
#include "Booking.h"
#include "CarPark.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <optional>
#include <pqxx/pqxx>
#include <unordered_map>
#include <vector>

struct LotActivityCounts
{
	int normalOccupied;
	int normalTotal;
	int disabledOccupied;
	int disabledTotal;
	int reservedOccupied;
	int reservedTotal;
};

class Database
{
	friend class Simulation;

private:
	mutable std::recursive_mutex m_dbMutex;
	std::unique_ptr<pqxx::connection> m_connection;
	std::unordered_map<std::string, std::unique_ptr<Account>> m_accounts;

	bool connect(std::string& host, std::string& port, std::string& dbname, std::string& user, std::string& password);
	bool loadAccounts();
	bool createSchema();
	void refreshBookingStatuses();
	std::unordered_map<int, std::unique_ptr<Lot>> loadLots();
	std::unordered_map<int, std::unique_ptr<Space>> loadNormalSpaces(int lotId);
	std::unordered_map<int, std::unique_ptr<DisabledSpace>> loadDisabledSpaces(int lotId);
	std::unordered_map<int, std::unique_ptr<ReservedSpace>> loadReservedSpaces(int lotId);
	std::vector<TempBooking> loadBookings(int lotId);

public:
	Database(std::string host, std::string port, std::string dbname, std::string user, std::string password);
	
	std::unique_ptr<CarPark> loadCarPark();
	bool isConnected() const;
	bool createAccount(const std::string& email, const std::string& password);
	bool validateAccount(const std::string& email, const std::string& password);
	bool accountExists(const std::string& email);
	bool insertBooking(int lotId, const std::string& email, const std::string& registration, std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end);
	bool cancelBookingRecord(int lotId, const std::string& email, const std::string& registration, std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end);
	bool updateBookingRecord(const std::string& email, int originalLotId, const std::string& originalRegistration, std::chrono::system_clock::time_point originalStart, std::chrono::system_clock::time_point originalEnd, int newLotId, const std::string& newRegistration, std::chrono::system_clock::time_point newStart, std::chrono::system_clock::time_point newEnd);
	std::vector<TempBooking> getUpcomingBookings(const std::string& email);
	std::vector<std::pair<int, int>> predictAvailableNormal(std::time_t futureTime, int lotId = 0);
	std::vector<std::pair<int, int>> predictAvailableDisabled(std::time_t futureTime, int lotId = 0);
	std::unordered_map<int, std::unordered_map<std::time_t, std::vector<int>>> getLotActivity();
	bool saveAvailabilitySnapshot(std::time_t snapshotTime, const std::unordered_map<int, std::vector<int>>& availabilityByLot);
};
