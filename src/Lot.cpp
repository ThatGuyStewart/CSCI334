#include "Lot.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
	struct BookingEvent
	{
		std::chrono::system_clock::time_point time;
		int delta;
	};
}

TicketMachine& Lot::getTicketMachine()
{
	return *m_ticketMachine;
}

Space& Lot::getSpace(int spaceId)
{
	{
		auto it = m_normalSpaces.find(spaceId);
		if (it != m_normalSpaces.end()) return *(it->second);
	}

	{
		auto it = m_disabledSpaces.find(spaceId);
		if (it != m_disabledSpaces.end()) return *(it->second);
	}

	{
		auto it = m_reservedSpaces.find(spaceId);
		if (it != m_reservedSpaces.end()) return *(it->second);
	}

	throw std::runtime_error("Space not found");
}

Lot::Lot(
	int lotId,
	std::unordered_map<int, std::unique_ptr<Space>>&& normalSpaces,
	std::unordered_map<int, std::unique_ptr<DisabledSpace>>&& disabledSpaces,
	std::unordered_map<int, std::unique_ptr<ReservedSpace>>&& reservedSpaces)
	: m_lotId(lotId),
	  m_normalSpaces(std::move(normalSpaces)),
	  m_disabledSpaces(std::move(disabledSpaces)),
	  m_reservedSpaces(std::move(reservedSpaces))
{
	m_ticketMachine = std::make_unique<TicketMachine>(lotId);
}

int Lot::getLotId() const
{
	return m_lotId;
}

int Lot::getReservedCapacity() const
{
	return static_cast<int>(m_reservedSpaces.size());
}

bool Lot::canCreateBooking(
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end) const
{
	if (start >= end) return false;

	const int capacity = static_cast<int>(m_reservedSpaces.size());
	if (capacity <= 0) return false;

	std::vector<BookingEvent> events;
	events.reserve((m_bookings.size() + 1) * 2);

	for (const auto& [email, booking] : m_bookings)
	{
		if (!booking) continue;
		if (!booking->isReserved(start, end)) continue;

		events.push_back({ booking->getStart(), +1 });
		events.push_back({ booking->getEnd(), -1 });
	}

	events.push_back({ start, +1 });
	events.push_back({ end, -1 });

	std::sort(
		events.begin(),
		events.end(),
		[](const BookingEvent& left, const BookingEvent& right)
		{
			if (left.time != right.time)
			{
				return left.time < right.time;
			}

			return left.delta < right.delta;
		});

	int concurrentBookings = 0;
	for (const auto& event : events)
	{
		concurrentBookings += event.delta;
		if (concurrentBookings > capacity)
		{
			return false;
		}
	}

	return true;
}

int Lot::findAvailableReservedSpace(
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
{
	if (!canCreateBooking(start, end))
	{
		return -1;
	}

	if (m_reservedSpaces.empty())
	{
		return -1;
	}

	return m_reservedSpaces.begin()->first;
}

bool Lot::createBooking(
	std::string email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
{

	if (!canCreateBooking(start, end))
	{
		return false;
	}

	const std::string emailKey = email;

	m_bookings.emplace(
		emailKey,
		std::make_unique<Booking>(
			m_lotId,
			std::move(email),
			std::move(registration),
			start,
			end));

	return true;
}

bool Lot::cancelBooking(
	int spaceId,
	std::string email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
{
	(void)spaceId;

	auto range = m_bookings.equal_range(email);
	for (auto it = range.first; it != range.second; ++it)
	{
		if (it->second && it->second->matches(email, registration, start, end))
		{
			m_bookings.erase(it);
			return true;
		}
	}

	return false;
}

int Lot::getNumberOfAvailableNormal()
{
	int count = 0;
	for (const auto& [id, space] : m_normalSpaces)
	{
		if (!space->isOccupied()) count++;
	}
	return count;
}

int Lot::getNumberOfAvailableDisabled()
{
	int count = 0;
	for (const auto& [id, space] : m_disabledSpaces)
	{
		if (!space->isOccupied()) count++;
	}
	return count;
}

std::vector<std::pair<int, bool>> Lot::getAvailableNormal()
{
	std::vector<std::pair<int, bool>> available;
	for (const auto& [id, space] : m_normalSpaces)
	{
		available.push_back(std::make_pair(id, !space->isOccupied()));
	}
	return available;
}

std::vector<std::pair<int, bool>> Lot::getAvailableDisabled()
{
	std::vector<std::pair<int, bool>> available;
	for (const auto& [id, space] : m_disabledSpaces)
	{
		available.push_back(std::make_pair(id, !space->isOccupied()));
	}
	return available;
}

bool Lot::bookingExists(
	std::string email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
{
	auto range = m_bookings.equal_range(email);
	for (auto it = range.first; it != range.second; ++it)
	{
		if (it->second && it->second->matches(email, registration, start, end))
		{
			return true;
		}
	}
	return false;
}

void Lot::removeExpiredBookings()
{
	const auto currentTime = std::chrono::system_clock::now();

	for (auto it = m_bookings.begin(); it != m_bookings.end(); )
	{
		if (it->second && it->second->getEnd() <= currentTime)
		{
			it = m_bookings.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Lot::removeExpiredTickets()
{
	if (m_ticketMachine)
	{
		m_ticketMachine->removeExpiredTickets();
	}
}

int Lot::getParkedWithoutTicket()
{
	int parked = 0;

	for (const auto& [id, space] : m_normalSpaces)
	{
		if (space->isOccupied()) parked++;
	}

	for (const auto& [id, space] : m_disabledSpaces)
	{
		if (space->isOccupied()) parked++;
	}

	for (const auto& [id, space] : m_reservedSpaces)
	{
		if (space->isOccupied()) parked++;
	}

	return parked - m_ticketMachine->getNumberOfTickets();
}

