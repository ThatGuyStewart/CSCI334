#pragma once

#include "Database.h"
#include "ThreadControl.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class CarParkService
{
	friend class Simulation;

	private:
		Database& m_database;
		std::unique_ptr<CarPark> m_carPark;
		ThreadControl m_availabilityUpdater;
		mutable std::mutex m_bookingMessageMutex;
		std::string m_lastBookingFailureMessage;

		void loadCarPark();
		void startAvailabilityUpdater();
		void stopAvailabilityUpdater();
		void setLastBookingFailureMessage(const std::string& message);

	public:
		explicit CarParkService(Database& database);
		~CarParkService();

		bool hasCarPark() const;

		bool isConnected() const;
		bool updateAvailabilityData();
		bool validateAccount(const std::string& email, const std::string& password);
		bool accountExists(const std::string& email);
		bool createAccount(const std::string& email, const std::string& password);
		std::vector<int> findAvailableReservedLots(
			std::chrono::system_clock::time_point start,
			std::chrono::system_clock::time_point end);

		bool createBooking(
			const std::string& email,
			std::string registration,
			std::chrono::system_clock::time_point start,
			std::chrono::system_clock::time_point end,
			int lotId);

		std::vector<TempBooking> getUpcomingBookings(const std::string& email);
		bool cancelBooking(
			const std::string& email,
			std::string registration,
			std::chrono::system_clock::time_point start,
			std::chrono::system_clock::time_point end,
			int lotId);

		bool updateBooking(
			const std::string& email,
			std::string originalRegistration,
			std::chrono::system_clock::time_point originalStart,
			std::chrono::system_clock::time_point originalEnd,
			int originalLotId,
			std::string newRegistration,
			std::chrono::system_clock::time_point newStart,
			std::chrono::system_clock::time_point newEnd,
			int newLotId);

		std::string getLastBookingFailureMessage() const;
		std::vector<std::pair<int, int>> predictAvailableNormal(std::time_t futureTime, int lotId = 0);
		std::vector<std::pair<int, int>> predictAvailableDisabled(std::time_t futureTime, int lotId = 0);
		std::vector<std::pair<int, int>> predictAvailableReserved(std::time_t futureTime, int lotId = 0);
		std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> getAvailableNormal(int lotId = 0);
		std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> getAvailableDisabled(int lotId = 0);
		std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> getAvailableReserved(int lotId = 0);
		std::unordered_map<int, std::unordered_map<std::time_t, std::vector<int>>> getLotActivity(std::time_t startTime, std::time_t endTime);
		bool isAdminAccount(const std::string& email);
		std::unordered_map<int, int> getParkedWithoutTicketCounts();
		std::vector<TempBooking> getBookingsForLot(int lotId);
		std::vector<TempBooking> getBookingsForLot(int lotId, std::time_t timePoint);
		int getReservedCapacity(int lotId) const;
		void stop();
};

