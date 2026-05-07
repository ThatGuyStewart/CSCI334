#pragma once
#include <chrono>
#include <string>

// Struct used by both CarPark and Database, represents a booking for a lot.
// Contains email, registration, start and end time of the booking.
struct TempBooking
{
	int lotId;
	std::string m_email;
	std::string m_registration;
	std::chrono::system_clock::time_point m_start;
	std::chrono::system_clock::time_point m_end;
};

class Booking
{
private:
	int m_lotId;
	std::string m_email;
	std::string m_registration;
	std::chrono::system_clock::time_point m_start;
	std::chrono::system_clock::time_point m_end;

public:
	Booking(
		int lotId,
		std::string email,
		std::string registration,
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end);

	int getLotId() const;
	std::string getEmail() const;
	std::string getRegistration() const;
	std::chrono::system_clock::time_point getStart() const;
	std::chrono::system_clock::time_point getEnd() const;

	bool isReserved(
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end) const;

	bool matches(
		std::string email,
		std::string registration,
		std::chrono::system_clock::time_point start,
		std::chrono::system_clock::time_point end) const;
};



