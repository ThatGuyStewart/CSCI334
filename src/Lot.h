#pragma once
#include "Space.h"
#include "Booking.h"
#include "TicketMachine.h"
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Lot
{
	friend class Simulation;

private:
	int m_lotId;
	std::unordered_map<int, std::unique_ptr<Space>> m_normalSpaces;
	std::unordered_map<int, std::unique_ptr<DisabledSpace>> m_disabledSpaces;
	std::unordered_map<int, std::unique_ptr<ReservedSpace>> m_reservedSpaces;
	std::unordered_multimap<std::string, std::unique_ptr<Booking>> m_bookings;
	std::unique_ptr<TicketMachine> m_ticketMachine;
	Space& getSpace(int spaceId);
	bool canCreateBooking(std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end) const;

public:
	Lot(
		int lotId,
		std::unordered_map<int, std::unique_ptr<Space>>&& normalSpaces,
		std::unordered_map<int, std::unique_ptr<DisabledSpace>>&& disabledSpaces,
		std::unordered_map<int, std::unique_ptr<ReservedSpace>>&& reservedSpaces);

	int getLotId() const;
	int	getReservedCapacity() const;

	int findAvailableReservedSpace(
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end);

	bool createBooking(
		std::string email,
		std::string registration,
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end);

	bool cancelBooking(
		int spaceId,
		std::string email,
		std::string registration,
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end);

	int getNumberOfAvailableNormal();
	int getNumberOfAvailableDisabled();
	std::vector<std::pair<int, bool>> getAvailableNormal();
	std::vector<std::pair<int, bool>> getAvailableDisabled();

	bool bookingExists(
		std::string email,
		std::string registration,
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end);

	void removeExpiredBookings();
	void removeExpiredTickets();
	int getParkedWithoutTicket();
};