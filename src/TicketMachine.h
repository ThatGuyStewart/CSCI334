#pragma once
#include "Ticket.h"

#include <vector>
#include <memory>
#include <chrono>
#include <mutex>

class TicketMachine
{
	friend class Simulation; // Allow Simulation to access private members for testing purposes

private:
	int m_lotId;
	std::vector<std::unique_ptr<Ticket>> m_tickets;
	mutable std::mutex m_ticketsMutex;

public:
	TicketMachine(int lotId);
	TicketMachine(const TicketMachine&) = delete;
	TicketMachine& operator=(const TicketMachine&) = delete;

	int getLotId() const;
	int getNumberOfTickets() const;
	void issueTicket(std::chrono::system_clock::time_point issueTime, std::chrono::system_clock::time_point expiryTime);
	void removeExpiredTickets();
};

