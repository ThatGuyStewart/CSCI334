#include "TicketMachine.h"

TicketMachine::TicketMachine(int lotId)
	: m_lotId(lotId)
{
}

int TicketMachine::getLotId() const
{
	return m_lotId;
}

int TicketMachine::getNumberOfTickets() const
{
	std::lock_guard<std::mutex> lock(m_ticketsMutex);
	return static_cast<int>(m_tickets.size());
}

void TicketMachine::issueTicket(std::chrono::system_clock::time_point issueTime, std::chrono::system_clock::time_point expiryTime)
{
	std::lock_guard<std::mutex> lock(m_ticketsMutex);
	m_tickets.push_back(std::make_unique<Ticket>(issueTime, expiryTime));
}

void TicketMachine::removeExpiredTickets()
{
	std::lock_guard<std::mutex> lock(m_ticketsMutex);

	const auto currentTime = std::chrono::system_clock::now();
	for (auto it = m_tickets.begin(); it != m_tickets.end();)
	{
		if ((*it)->getExpiryTime() <= currentTime)
		{
			it = m_tickets.erase(it);
		}
		else
		{
			++it;
		}
	}
}
