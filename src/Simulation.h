#pragma once
#include "Server.h"
#include "Database.h"
#include "CarParkService.h"
#include "ThreadControl.h"
#include <chrono>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

struct OccupancyParameters
{
	double normalMin;
	double normalMax;
	double disabledMin;
	double disabledMax;
};

struct CandidateChange
{
	int lotId;
	int spaceId;
	bool occupied;
	int priority;
};

struct SimulatedTicketInfo
{
	bool hasTicket;
	std::chrono::system_clock::time_point expectedDepartureTime;
	std::chrono::system_clock::time_point ticketExpiryTime;
};

class Simulation
{
private:
	Server& m_server;
	CarParkService& m_service;
	Database& m_db;
	std::mt19937 m_rng;
	ThreadControl m_threadControl;
	std::unordered_map<long long, SimulatedTicketInfo> m_spaceTicketInfo;

	void simulateParkingBehavior();
	OccupancyParameters getOccupancyParameters(const std::tm& localTm) const;
	std::vector<CandidateChange> buildCandidateChanges(
		const std::vector<std::pair<int, std::vector<std::pair<int, bool>>>>& lots,
		double minOccupancy,
		double maxOccupancy);

	void seedDatabaseWithHistoricalData();
	bool trySeedHistoricalBooking(
		pqxx::work& tx,
		const std::vector<std::string>& emails,
		std::unordered_map<std::string, std::vector<std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point>>>& bookingsByEmail,
		int lotId,
		const std::string& registrationPrefix,
		int& bookingCounter,
		std::chrono::system_clock::time_point startTime,
		std::chrono::system_clock::time_point endTime);

	int getHistoricalReservedBookingTarget(const std::tm& localTm) const;
	long long getSpaceKey(int lotId, int spaceId) const;
	void handleSpaceBecomingOccupied(Lot& lot, Space& space);
	void handleSpaceBecomingAvailable(Lot& lot, Space& space);
	void syncReservedSpacesWithBookings(std::chrono::system_clock::time_point currentTime);

public:
	Simulation(Server& server, CarParkService& service, Database& db);
	void run();
	void stop();
};