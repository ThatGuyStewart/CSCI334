#include "CarPark.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>

CarPark::CarPark(std::unordered_map<int, std::unique_ptr<Lot>>&& lots)
	: m_lots(std::move(lots))
{
	startTicketCleanupThread();
}

CarPark::~CarPark()
{
	stopTicketCleanupThread();
}

void CarPark::startTicketCleanupThread()
{
	if (m_ticketCleanupThread.thread.joinable()) return;

	m_ticketCleanupThread.running.store(true);
	m_ticketCleanupThread.thread = std::thread([this]()
	{
		std::unique_lock<std::mutex> lock(m_ticketCleanupThread.mutex);

		while (m_ticketCleanupThread.running.load())
		{
			const bool stopping = m_ticketCleanupThread.cv.wait_for(
				lock,
				std::chrono::minutes(1),
				[this]() { return !m_ticketCleanupThread.running.load(); });

			if (stopping) break;

			lock.unlock();
			removeExpiredTickets();
			lock.lock();
		}
	});
}

void CarPark::stopTicketCleanupThread()
{
	if (!m_ticketCleanupThread.running.exchange(false)) return;

	m_ticketCleanupThread.cv.notify_all();

	if (m_ticketCleanupThread.thread.joinable())
	{
		m_ticketCleanupThread.thread.join();
	}
}

void CarPark::removeExpiredTickets()
{
	for (const auto& [id, lot] : m_lots)
	{
		if (lot)
		{
			lot->removeExpiredTickets();
		}
	}
}

// If lotId is specified, only check that lot for availability. Otherwise check all lots and return a list of those with availability.
// Returns a vector of lot IDs that have availability for the given time range. Returns an empty vector if no lots have availability.
std::vector<int> CarPark::findAvailableReservedLots(
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
{
	std::vector<int> freeLots;

	for (const auto& [id, lot] : m_lots)
	{
		if (!lot) continue;

		const int bookingSlot = lot->findAvailableReservedSpace(start, end);
		if (bookingSlot != -1)
		{
			freeLots.push_back(id);
		}
	}

	return freeLots;
}

int CarPark::findAvailableReservedSpace(
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end,
	int lotId)
{
	if (lotId <= 0) return -1;

	auto it = m_lots.find(lotId);
	if (it == m_lots.end() || !it->second) return -1;

	return it->second->findAvailableReservedSpace(start, end);
}

bool CarPark::createBooking(
	int lotId,
	std::string email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
{
	if (lotId <= 0) return false;

	auto it = m_lots.find(lotId);
	if (it == m_lots.end() || !it->second) return false;

	return it->second->createBooking(std::move(email), std::move(registration), start, end);
}

bool CarPark::cancelBooking(
	int lotId,
	std::string email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
{
	if (lotId <= 0) return false;

	auto it = m_lots.find(lotId);
	if (it == m_lots.end() || !it->second) return false;

	return it->second->cancelBooking(0, std::move(email), std::move(registration), start, end);
}

int CarPark::getReservedCapacity(int lotId) const
{
	if (lotId <= 0) return 0;

	auto it = m_lots.find(lotId);
	if (it == m_lots.end() || !it->second) return 0;

   return it->second->getReservedCapacity();
}

std::vector<std::pair<int, int>> CarPark::getNumberOfAvailableNormal(int lotId)
{
	std::vector<std::pair<int, int>> availability;

	if (lotId == 0)
	{
		for (const auto& [id, lot] : m_lots)
		{
			if (lot)
			{
				availability.push_back(std::make_pair(id, lot->getNumberOfAvailableNormal()));
			}
		}
		return availability;
	}

	auto it = m_lots.find(lotId);
	if (it != m_lots.end() && it->second)
	{
		availability.push_back(std::make_pair(lotId, it->second->getNumberOfAvailableNormal()));
	}

	return availability;
}

std::vector<std::pair<int, int>> CarPark::getNumberOfAvailableDisabled(int lotId)
{
	std::vector<std::pair<int, int>> availability;

	if (lotId == 0)
	{
		for (const auto& [id, lot] : m_lots)
		{
			if (lot)
			{
				availability.push_back(std::make_pair(id, lot->getNumberOfAvailableDisabled()));
			}
		}
		return availability;
	}

	auto it = m_lots.find(lotId);
	if (it != m_lots.end() && it->second)
	{
		availability.push_back(std::make_pair(lotId, it->second->getNumberOfAvailableDisabled()));
	}

	return availability;
}

std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> CarPark::getAvailableNormal(int lotId)
{
	std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> availability;

	if (lotId == 0)
	{
		for (const auto& [id, lot] : m_lots)
		{
			if (lot)
			{
				availability.push_back(std::make_pair(id, lot->getAvailableNormal()));
			}
		}
		return availability;
	}

	auto it = m_lots.find(lotId);
	if (it != m_lots.end() && it->second)
	{
		availability.push_back(std::make_pair(lotId, it->second->getAvailableNormal()));
	}

	return availability;
}

std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> CarPark::getAvailableDisabled(int lotId)
{
	std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> availability;

	if (lotId == 0)
	{
		for (const auto& [id, lot] : m_lots)
		{
			if (lot)
			{
				availability.push_back(std::make_pair(id, lot->getAvailableDisabled()));
			}
		}
		return availability;
	}

	auto it = m_lots.find(lotId);
	if (it != m_lots.end() && it->second)
	{
		availability.push_back(std::make_pair(lotId, it->second->getAvailableDisabled()));
	}

	return availability;
}

bool CarPark::bookingExists(
	int lotId,
	std::string email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
{
	if (lotId > 0)
	{
		auto it = m_lots.find(lotId);
		if (it != m_lots.end() && it->second)
		{
			return it->second->bookingExists(std::move(email), std::move(registration), start, end);
		}
		return false;
	}
	for (const auto& [id, lot] : m_lots)
	{
		if (lot && lot->bookingExists(email, registration, start, end))
		{
			return true;
		}
	}
	return false;
}

void CarPark::removeExpiredBookings()
{
	for (const auto& [id, lot] : m_lots)
	{
		if (lot)
		{
			lot->removeExpiredBookings();
		}
	}
}
