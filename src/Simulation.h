#pragma once
#include "Server.h"
#include "Database.h"
#include "CarParkService.h"
#include "ThreadControl.h"
#include <random>
#include <vector>
#include <utility>

struct OccupancyParameters
{
	double normalMin;
	double normalMax;
	double disabledMin;
	double disabledMax;
};

class Simulation
{
private:
	struct CandidateChange
	{
		int lotId;
		int spaceId;
		bool occupied;
		int priority;
	};

	Server& m_server;
	CarParkService& m_service;
	Database& m_db;
	std::mt19937 m_rng;
	ThreadControl m_threadControl;

	void simulateParkingBehavior();
	OccupancyParameters getOccupancyParameters(const std::tm& localTm) const;
	std::vector<CandidateChange> buildCandidateChanges(
		const std::vector<std::pair<int, std::vector<std::pair<int, bool>>>>& lots,
		double minOccupancy,
		double maxOccupancy);
	void seedDatabaseWithHistoricalData();

public:
	Simulation(Server& server, CarParkService& service, Database& db);
	void run();
	void stop();
};

