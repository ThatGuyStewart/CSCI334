#include "Booking.h"
#include <cstdlib>

Booking::Booking(
	int lotId,
	std::string email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end)
	: m_lotId(lotId),
	  m_email(std::move(email)),
	  m_registration(std::move(registration)),
	  m_start(start),
	  m_end(end)
{
}

int Booking::getLotId() const
{
	return m_lotId;
}

std::string Booking::getEmail() const
{
	return m_email;
}

std::string Booking::getRegistration() const
{
	return m_registration;
}

std::chrono::system_clock::time_point Booking::getStart() const
{
	return m_start;
}

std::chrono::system_clock::time_point Booking::getEnd() const
{
	return m_end;
}

bool Booking::isReserved(
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end) const
{
	return start < m_end && end > m_start;
}

bool Booking::matches(
	std::string email,
	std::string registration,
	std::chrono::system_clock::time_point start,
	std::chrono::system_clock::time_point end) const
{
	const auto thisStartSeconds =
		std::chrono::duration_cast<std::chrono::seconds>(m_start.time_since_epoch()).count();
	const auto thisEndSeconds =
		std::chrono::duration_cast<std::chrono::seconds>(m_end.time_since_epoch()).count();
	const auto startSeconds =
		std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count();
	const auto endSeconds =
		std::chrono::duration_cast<std::chrono::seconds>(end.time_since_epoch()).count();

	const auto startDifference = std::llabs(thisStartSeconds - startSeconds);
	const auto endDifference = std::llabs(thisEndSeconds - endSeconds);

	return m_email == email &&
		m_registration == registration &&
		startDifference <= 1 &&
		endDifference <= 1;
}

