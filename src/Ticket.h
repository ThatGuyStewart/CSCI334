#pragma once
#include <chrono>

class Ticket
{
private:
	std::chrono::system_clock::time_point m_issueTime;
	std::chrono::system_clock::time_point m_expiryTime;
public:
	Ticket(std::chrono::system_clock::time_point issueTime, std::chrono::system_clock::time_point expiryTime);
	std::chrono::system_clock::time_point getIssueTime() const;
	std::chrono::system_clock::time_point getExpiryTime() const;
};

