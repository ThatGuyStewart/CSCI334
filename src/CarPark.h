#pragma once
#include "Lot.h"
#include "ThreadControl.h"
#include <memory>
#include <vector>
#include <chrono>
#include <string>
#include <utility>
#include <unordered_map>

class CarPark
{
	friend class Simulation;

private:
	std::unordered_map<int, std::unique_ptr<Lot>> m_lots;
	ThreadControl m_ticketCleanupThread;

	void startTicketCleanupThread();
	void stopTicketCleanupThread();

public:
	CarPark(const CarPark&) = delete;
	CarPark& operator=(const CarPark&) = delete;
	CarPark(CarPark&&) noexcept = default;
	CarPark& operator=(CarPark&&) noexcept = default;

	CarPark(std::unordered_map<int, std::unique_ptr<Lot>>&& lots);
	~CarPark();

	std::vector<int> findAvailableReservedLots(
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end);

	int findAvailableReservedSpace(
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end,
		int lotId);

	bool createBooking(
		std::string email,
		std::string registration,
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end,
		int lotId);

	bool cancelBooking(
		std::string email,
		std::string registration,
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end,
		int lotId);

	bool bookingExists(
		std::string email,
		std::string registration,
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end,
		int lotId);

	int getReservedCapacity(int lotId) const;

	std::vector<std::pair<int, int>> getNumberOfAvailableNormal(int lotId = 0);
	std::vector<std::pair<int, int>> getNumberOfAvailableDisabled(int lotId = 0);
	std::vector<std::pair<int, int>> getNumberOfAvailableReserved(int lotId = 0);
	std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> getAvailableNormal(int lotId = 0);
	std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> getAvailableDisabled(int lotId = 0);
	void removeExpiredBookings();
	void removeExpiredTickets();
	std::unordered_map<int, int> getParkedWithoutTicketByLot();
};

