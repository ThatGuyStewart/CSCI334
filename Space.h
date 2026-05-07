#pragma once

#include <chrono>

class Space
{
protected:
	int m_spaceId;
	int m_lotId;
	bool m_occupied;
	std::chrono::system_clock::time_point m_occupiedSince;

public:
	Space(int lotId, int spaceId);
	int getLotId() const;
	int getSpaceId() const;
	bool isOccupied() const;
	bool canBecomeAvailable(std::chrono::system_clock::time_point currentTime) const;
	void setOccupied(bool occupied);
	bool changeOccupied();
};

class DisabledSpace : public Space
{
public:
	DisabledSpace(int lotId, int spaceId);
};

class ReservedSpace : public Space
{
public:
	ReservedSpace(int lotId, int spaceId);
	~ReservedSpace();
};