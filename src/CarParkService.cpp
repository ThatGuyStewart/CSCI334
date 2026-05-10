#include "CarParkService.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>

CarParkService::CarParkService(Database& database)
	: m_database(database), m_carPark(nullptr)
{
}

CarParkService::~CarParkService()
{
	stopAvailabilityUpdater();
}

void CarParkService::setLastBookingFailureMessage(const std::string& message)
{
	std::lock_guard<std::mutex> lock(m_bookingMessageMutex);
	m_lastBookingFailureMessage = message;
}

std::string CarParkService::getLastBookingFailureMessage() const
{
	std::lock_guard<std::mutex> lock(m_bookingMessageMutex);
	return m_lastBookingFailureMessage;
}

void CarParkService::loadCarPark()
{
	m_carPark = m_database.loadCarPark();

	if (m_carPark)
	{
		startAvailabilityUpdater();
	}
}

bool CarParkService::hasCarPark() const
{
	return m_carPark != nullptr;
}

bool CarParkService::isConnected() const
{
	return m_database.isConnected();
}

void CarParkService::startAvailabilityUpdater()
{
	if (m_availabilityUpdater.thread.joinable()) return;

	m_availabilityUpdater.running.store(true);
	updateAvailabilityData();

	m_availabilityUpdater.thread = std::thread([this]()
	{
		std::unique_lock<std::mutex> lock(m_availabilityUpdater.mutex);

		while (m_availabilityUpdater.running.load())
		{
			const bool stopping = m_availabilityUpdater.cv.wait_for(
				lock,
				std::chrono::minutes(1),
				[this]() { return !m_availabilityUpdater.running.load(); });

			if (stopping) break;

			lock.unlock();
			updateAvailabilityData();
			lock.lock();
		}
	});
}

void CarParkService::stopAvailabilityUpdater()
{
	if (!m_availabilityUpdater.running.exchange(false)) return;

	m_availabilityUpdater.cv.notify_all();

	if (m_availabilityUpdater.thread.joinable())
	{
		m_availabilityUpdater.thread.join();
	}
}

bool CarParkService::updateAvailabilityData()
{
	if (!m_carPark) return false;
	if (!m_database.isConnected()) return false;

	try
	{
		const auto normalAvailability = m_carPark->getNumberOfAvailableNormal(0);
		const auto disabledAvailability = m_carPark->getNumberOfAvailableDisabled(0);
		const auto snapshotTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		std::unordered_map<int, std::vector<int>> availabilityByLot;
		availabilityByLot.reserve(std::max(normalAvailability.size(), disabledAvailability.size()));

		for (const auto& [lotId, availableNormalSpaces] : normalAvailability)
		{
			availabilityByLot[lotId] = { availableNormalSpaces, 0 };
		}

		for (const auto& [lotId, availableDisabledSpaces] : disabledAvailability)
		{
			if (availabilityByLot.find(lotId) == availabilityByLot.end())
			{
				availabilityByLot[lotId] = { 0, availableDisabledSpaces };
			}
			else
			{
				availabilityByLot[lotId][1] = availableDisabledSpaces;
			}
		}

		return m_database.saveAvailabilitySnapshot(snapshotTime, availabilityByLot);
	}
	catch (const std::exception& e)
	{
		std::cout << "Update availability data failed: " << e.what() << std::endl;
		return false;
	}
}

bool CarParkService::validateAccount(const std::string& email, const std::string& password)
{
	return m_database.validateAccount(email, password);
}

bool CarParkService::accountExists(const std::string& email)
{
	return m_database.accountExists(email);
}

bool CarParkService::createAccount(const std::string& email, const std::string& password)
{
	return m_database.createAccount(email, password);
}

std::vector<int> CarParkService::findAvailableReservedLots(
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
{
	if (!m_carPark) return {};
	if (start >= end) return {};

	return m_carPark->findAvailableReservedLots(start, end);
}

bool CarParkService::createBooking(
	const std::string& email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end,
	int lotId)
{
	setLastBookingFailureMessage("");

	if (!m_carPark)
	{
		setLastBookingFailureMessage("Car park is not loaded.");
		return false;
	}

	if (registration.empty())
	{
		setLastBookingFailureMessage("Registration is required.");
		return false;
	}

	if (start >= end)
	{
		setLastBookingFailureMessage("Booking end time must be after the start time.");
		return false;
	}

	if ((end - start) > std::chrono::hours(8))
	{
		setLastBookingFailureMessage("Bookings cannot exceed 8 hours.");
		return false;
	}

	if (lotId <= 0)
	{
		setLastBookingFailureMessage("A valid lot must be selected.");
		return false;
	}

	if (!m_database.accountExists(email))
	{
		setLastBookingFailureMessage("Account does not exist.");
		return false;
	}

	if (m_carPark->findAvailableReservedSpace(start, end, lotId) == -1)
	{
		setLastBookingFailureMessage("No reserved spaces are available for that time range.");
		return false;
	}

	if (!m_database.insertBooking(email, registration, start, end, lotId))
	{
		setLastBookingFailureMessage("Booking could not be created. You may already have an overlapping booking or 5 active bookings.");
		return false;
	}

	if (!m_carPark->createBooking(email, std::move(registration), start, end, lotId))
	{
		setLastBookingFailureMessage("Booking was saved but could not be added to live memory.");
		return false;
	}

	return true;
}

std::vector<TempBooking> CarParkService::getUpcomingBookings(const std::string& email)
{
	return m_database.getUpcomingBookings(email);
}

bool CarParkService::cancelBooking(
	const std::string& email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end,
	int lotId)
{
	if (!m_carPark) return false;
	if (lotId <= 0) return false;
	if (registration.empty()) return false;
	if (start >= end) return false;

	if (!m_database.cancelBookingRecord(email, registration, start, end, lotId))
	{
		return false;
	}

	if (!m_carPark->cancelBooking(email, std::move(registration), start, end, lotId))
	{
		std::cout << "Cancel booking warning: database updated but in-memory booking was not found." << std::endl;
	}

	return true;
}

bool CarParkService::updateBooking(
	const std::string& email,
	std::string originalRegistration,
	std::chrono::system_clock::time_point originalStart,
	std::chrono::system_clock::time_point originalEnd,
	int originalLotId,
	std::string newRegistration,
	std::chrono::system_clock::time_point newStart,
	std::chrono::system_clock::time_point newEnd,
	int newLotId)
{
	setLastBookingFailureMessage("");

	if (!m_carPark)
	{
		setLastBookingFailureMessage("Car park is not loaded.");
		return false;
	}

	if (originalLotId <= 0 || newLotId <= 0)
	{
		setLastBookingFailureMessage("A valid lot must be selected.");
		return false;
	}

	if (originalRegistration.empty() || newRegistration.empty())
	{
		setLastBookingFailureMessage("Registration is required.");
		return false;
	}

	if (originalStart >= originalEnd || newStart >= newEnd)
	{
		setLastBookingFailureMessage("Booking end time must be after the start time.");
		return false;
	}

	if ((newEnd - newStart) > std::chrono::hours(6))
	{
		setLastBookingFailureMessage("Bookings cannot exceed 6 hours.");
		return false;
	}

	const bool removedFromMemory = m_carPark->cancelBooking(
		email,
		originalRegistration,
		originalStart,
		originalEnd,
		originalLotId);

	if (!removedFromMemory)
	{
		std::cout << "Update booking warning: original in-memory booking was not found." << std::endl;
	}

	if (m_carPark->findAvailableReservedSpace(newStart, newEnd, newLotId) == -1)
	{
		if (removedFromMemory)
		{
			m_carPark->createBooking(
				email,
				originalRegistration,
				originalStart,
				originalEnd,
				originalLotId);
		}

		setLastBookingFailureMessage("No reserved spaces are available for that new time range.");
		return false;
	}

	if (!m_database.updateBookingRecord(
		email,
		originalRegistration,
		originalStart,
		originalEnd,
		originalLotId,
		newRegistration,
		newStart,
		newEnd,
		newLotId))
	{
		if (removedFromMemory)
		{
			m_carPark->createBooking(
				email,
				originalRegistration,
				originalStart,
				originalEnd,
				originalLotId);
		}

		setLastBookingFailureMessage("Booking could not be updated. You may already have an overlapping booking or 5 active bookings.");
		return false;
	}

	if (!m_carPark->createBooking(email, std::move(newRegistration), newStart, newEnd, newLotId))
	{
		std::cout << "Update booking warning: database updated but in-memory booking could not be inserted." << std::endl;
		setLastBookingFailureMessage("Booking was updated but could not be added to live memory.");
		return false;
	}

	return true;
}

std::vector<std::pair<int, int>> CarParkService::predictAvailableNormal(std::time_t futureTime, int lotId)
{
	return m_database.predictAvailableNormal(futureTime, lotId);
}

std::vector<std::pair<int, int>> CarParkService::predictAvailableDisabled(std::time_t futureTime, int lotId)
{
	return m_database.predictAvailableDisabled(futureTime, lotId);
}

std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> CarParkService::getAvailableNormal(int lotId)
{
	if (!m_carPark) return {};
	return m_carPark->getAvailableNormal(lotId);
}

std::vector<std::pair<int, std::vector<std::pair<int, bool>>>> CarParkService::getAvailableDisabled(int lotId)
{
	if (!m_carPark) return {};
	return m_carPark->getAvailableDisabled(lotId);
}

std::unordered_map<int, std::unordered_map<std::time_t, std::vector<int>>> CarParkService::getLotActivity()
{
	return m_database.getLotActivity();
}

void CarParkService::stop()
{
	stopAvailabilityUpdater();
}

bool CarParkService::isAdminAccount(const std::string& email)
{
	return m_database.isAdminAccount(email);
}

std::unordered_map<int, int> CarParkService::getParkedWithoutTicketCounts()
{
	return m_carPark ? m_carPark->getParkedWithoutTicketByLot() : std::unordered_map<int, int>{};
}

std::vector<TempBooking> CarParkService::getCurrentBookingsForLot(int lotId)
{
	return m_database.getCurrentBookingsForLot(lotId);
}

int CarParkService::getReservedCapacity(int lotId) const
{
	return m_carPark ? m_carPark->getReservedCapacity(lotId) : 0;
}
