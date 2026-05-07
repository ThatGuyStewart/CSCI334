#include "Space.h"

Space::Space(int lotId, int spaceId)
	: m_spaceId(spaceId), m_lotId(lotId), m_occupied(false), m_occupiedSince(std::chrono::system_clock::time_point::min())
{
}

DisabledSpace::DisabledSpace(int lotId, int spaceId)
	: Space(lotId, spaceId)
{
}

ReservedSpace::ReservedSpace(int lotId, int spaceId)
	: Space(lotId, spaceId)
{
}

ReservedSpace::~ReservedSpace() = default;

int Space::getLotId() const
{
	return m_lotId;
}

int Space::getSpaceId() const
{
	return m_spaceId;
}

bool Space::isOccupied() const
{
	return m_occupied;
}

bool Space::canBecomeAvailable(std::chrono::system_clock::time_point currentTime) const
{
	if (!m_occupied) return true;

	const auto minimumOccupiedDuration = std::chrono::minutes(1);
	return (currentTime - m_occupiedSince) >= minimumOccupiedDuration;
}

void Space::setOccupied(bool occupied)
{
	if (occupied)
	{
		if (!m_occupied)
		{
			m_occupied = true;
			m_occupiedSince = std::chrono::system_clock::now();
		}
		return;
	}

	if (!m_occupied) return;

	if (canBecomeAvailable(std::chrono::system_clock::now()))
	{
		m_occupied = false;
		m_occupiedSince = std::chrono::system_clock::time_point::min();
	}
}

bool Space::changeOccupied()
{
	if (!m_occupied)
	{
		m_occupied = true;
		m_occupiedSince = std::chrono::system_clock::now();
		return true;
	}

	if (!canBecomeAvailable(std::chrono::system_clock::now()))
	{
		return false;
	}

	m_occupied = false;
	m_occupiedSince = std::chrono::system_clock::time_point::min();
	return true;
}
