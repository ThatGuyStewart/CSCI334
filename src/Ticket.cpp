#include "Ticket.h"

Ticket::Ticket(std::chrono::system_clock::time_point issueTime, std::chrono::system_clock::time_point expiryTime)
	: m_issueTime(issueTime), m_expiryTime(expiryTime)
{
}

std::chrono::system_clock::time_point Ticket::getIssueTime() const
{
	return m_issueTime;
}

std::chrono::system_clock::time_point Ticket::getExpiryTime() const
{
	return m_expiryTime;
}
