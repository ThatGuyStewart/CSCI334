#include "Server.h"
#include <random>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <ctime>

namespace
{
	bool tryGetTimeParam(const httplib::Request& req, const char* name, std::time_t& value)
	{
		if (!req.has_param(name)) return false;

		try
		{
			value = static_cast<std::time_t>(std::stoll(req.get_param_value(name)));
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	int getLotIdParam(const httplib::Request& req)
	{
		if (!req.has_param("lotId")) return 0;

		try
		{
			return std::stoi(req.get_param_value("lotId"));
		}
		catch (...)
		{
			return 0;
		}
	}

	nlohmann::json buildAvailabilityMessage(CarParkService& service)
	{
		const auto normalAvailability = service.getAvailableNormal(0);
		const auto disabledAvailability = service.getAvailableDisabled(0);

		nlohmann::json normalLots = nlohmann::json::array();
		for (const auto& lotEntry : normalAvailability)
		{
			nlohmann::json spaces = nlohmann::json::array();

			for (const auto& spaceEntry : lotEntry.second)
			{
				spaces.push_back(
					{
						{"spaceId", spaceEntry.first},
						{"available", spaceEntry.second}
					});
			}

			normalLots.push_back(
				{
					{"lotId", lotEntry.first},
					{"spaces", spaces}
				});
		}

		nlohmann::json disabledLots = nlohmann::json::array();
		for (const auto& lotEntry : disabledAvailability)
		{
			nlohmann::json spaces = nlohmann::json::array();

			for (const auto& spaceEntry : lotEntry.second)
			{
				spaces.push_back(
					{
						{"spaceId", spaceEntry.first},
						{"available", spaceEntry.second}
					});
			}

			disabledLots.push_back(
				{
					{"lotId", lotEntry.first},
					{"spaces", spaces}
				});
		}

		return
		{
			{"type", "availability"},
			{"serverTime", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())},
			{"normalLots", normalLots},
			{"disabledLots", disabledLots}
		};
	}
}

Server::Server(CarParkService& service, const char* cert, const char* key, std::string address, int port)
	: httplib::SSLServer(cert, key), m_service(service), m_address(address), m_port(port)
{
	createRoutes();
}

Server::~Server()
{
	stop();
	if (m_broadcasterThread.thread.joinable())
	{
		m_broadcasterThread.thread.join();
	}
}

std::string Server::loadFile(const std::string& path) 
{
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) 
	{
		std::cout << "File " << path << " failed to load." << std::endl;
		return {};
	};
	std::ostringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

std::string Server::createToken() 
{
	static std::mt19937_64 rng((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<uint64_t> dist;
	std::ostringstream token;
	token << std::hex << dist(rng) << dist(rng);
	return token.str();
}

std::string Server::getTokenFromCookie(const httplib::Request& req) 
{
	std::string cookie = req.get_header_value("Cookie");
	auto pos = cookie.find("session=");
	if (pos == std::string::npos) return {};
	pos += strlen("session=");
	auto end = cookie.find(';', pos);
	return cookie.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

std::string Server::getUsernameForToken(const std::string& token) 
{
	if (token.empty()) return {};
	std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);
	auto it = m_sessions.tokenToUsername.find(token);
	if (it == m_sessions.tokenToUsername.end()) return {};
	return it->second;
}

bool Server::validateAccount(const std::string& email, const std::string& password) 
{
	return m_service.validateAccount(email, password);
}

void Server::createRoutes() 
{
	// Create static file routes
	createFileRoute("/test", "HTML/test.html", "text/html");
	createFileRoute("/Javascript/test-client.js", "HTML/Javascript/test-client.js", "application/javascript");

	// Create API routes
	Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) { handleApiLogin(req, res); });
	Get("/api/user", [&](const httplib::Request& req, httplib::Response& res) { handleApiUser(req, res); });
	Get("/api/updates", [&](const httplib::Request& req, httplib::Response& res) { handleApiUpdates(req, res); });
	Get("/api/carpark/predict/normal", [&](const httplib::Request& req, httplib::Response& res) { handleApiPredictNormal(req, res); });
	Get("/api/carpark/predict/disabled", [&](const httplib::Request& req, httplib::Response& res) { handleApiPredictDisabled(req, res); });
	Get("/api/carpark/available-reserved-lots", [&](const httplib::Request& req, httplib::Response& res) { handleApiAvailableReservedLots(req, res); });
	Get("/api/carpark/available/normal", [&](const httplib::Request& req, httplib::Response& res) { handleApiAvailableNormal(req, res); });
	Get("/api/carpark/available/disabled", [&](const httplib::Request& req, httplib::Response& res) { handleApiAvailableDisabled(req, res); });
	Post("/api/carpark/bookings", [&](const httplib::Request& req, httplib::Response& res) { handleApiCreateBooking(req, res); });
	Get("/api/bookings", [&](const httplib::Request& req, httplib::Response& res) { handleApiGetBookings(req, res); });
	Delete("/api/bookings", [&](const httplib::Request& req, httplib::Response& res) { handleApiCancelBooking(req, res); });
	Put("/api/bookings", [&](const httplib::Request& req, httplib::Response& res) { handleApiUpdateBooking(req, res); });

	createWebsocketRoute();
}

void Server::createFileRoute(const std::string& route, const std::string& path, const std::string& type) 
{
	Get(route.c_str(), [this, path, type](const httplib::Request& /*req*/, httplib::Response& res) 
		{
		auto body = loadFile(path);
		if (body.empty()) 
		{
			res.status = 404;
			res.set_content("Not found", "text/plain");
			return;
		}
		res.set_content(body, type);
		});
}

void Server::handleApiLogin(const httplib::Request& req, httplib::Response& res) 
{
	try 
	{
		auto j = nlohmann::json::parse(req.body);
		std::string user = j.value("username", "");
		std::string pass = j.value("password", "");

		if (validateAccount(user, pass)) 
		{
			std::string token = createToken();
			{
				std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);
				// Invalidate any existing session for this user
				auto oldToken = m_sessions.usernameToToken.find(user);
				if (oldToken != m_sessions.usernameToToken.end()) 
				{
					m_sessions.tokenToUsername.erase(oldToken->second);
				}
				m_sessions.tokenToUsername[token] = user;
				m_sessions.usernameToToken[user] = token;
			}
			res.set_header("Set-Cookie", std::string("session=") + token + "; Path=/; HttpOnly");
			nlohmann::json reply = { {"status", "ok"}, {"user", user} };
			res.set_content(reply.dump(), "application/json");
			std::cout << " User " << user << " logged in with token " << token << std::endl;
			return;
		}
		res.status = 401;
		res.set_content(R"({"error":"invalid credentials"})", "application/json");
	}
	catch (const nlohmann::json::parse_error&) 
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiUser(const httplib::Request& req, httplib::Response& res) 
{
	std::string token = getTokenFromCookie(req);
	if (!token.empty()) 
	{
		std::string user = getUsernameForToken(token);
		if (!user.empty()) 
		{
			nlohmann::json reply = { {"user", user} };
			res.set_content(reply.dump(), "application/json");
			return;
		}
	}
	res.status = 401;
	res.set_content(R"({"error":"unauthenticated"})", "application/json");
}

void Server::handleApiUpdates(const httplib::Request& req, httplib::Response& res)
{
	std::string token = getTokenFromCookie(req);
	std::string user = getUsernameForToken(token);

	if (token.empty() || user.empty())
	{
		res.status = 401;
		res.set_content(R"({"error":"unauthenticated"})", "application/json");
		return;
	}

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"user", user},
		{"serverTime", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
	};

	res.set_content(reply.dump(2), "application/json");
}

void Server::handleApiPredictNormal(const httplib::Request& req, httplib::Response& res)
{
	std::time_t futureTimeValue = 0;
	if (!tryGetTimeParam(req, "futureTime", futureTimeValue))
	{
		res.status = 400;
		res.set_content(R"({"error":"missing or invalid futureTime"})", "application/json");
		return;
	}

	const int lotId = getLotIdParam(req);
	const auto predictions = m_service.predictAvailableNormal(futureTimeValue, lotId);

	nlohmann::json items = nlohmann::json::array();
	for (const auto& prediction : predictions)
	{
		items.push_back(
			{
				{"lotId", prediction.first},
				{"availableSpaces", prediction.second}
			});
	}

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"predictions", items}
	};

	res.set_content(reply.dump(2), "application/json");
}

void Server::handleApiPredictDisabled(const httplib::Request& req, httplib::Response& res)
{
	std::time_t futureTimeValue = 0;
	if (!tryGetTimeParam(req, "futureTime", futureTimeValue))
	{
		res.status = 400;
		res.set_content(R"({"error":"missing or invalid futureTime"})", "application/json");
		return;
	}

	const int lotId = getLotIdParam(req);
	const auto predictions = m_service.predictAvailableDisabled(futureTimeValue, lotId);

	nlohmann::json items = nlohmann::json::array();
	for (const auto& prediction : predictions)
	{
		items.push_back(
			{
				{"lotId", prediction.first},
				{"availableSpaces", prediction.second}
			});
	}

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"predictions", items}
	};

	res.set_content(reply.dump(2), "application/json");
}

void Server::handleApiAvailableReservedLots(const httplib::Request& req, httplib::Response& res)
{
	std::string token = getTokenFromCookie(req);
	if (token.empty() || getUsernameForToken(token).empty())
	{
		res.status = 401;
		res.set_content(R"({"error":"unauthenticated"})", "application/json");
		return;
	}

	std::time_t startTimeValue = 0;
	std::time_t endTimeValue = 0;

	if (!tryGetTimeParam(req, "startTime", startTimeValue) || !tryGetTimeParam(req, "endTime", endTimeValue))
	{
		res.status = 400;
		res.set_content(R"({"error":"missing or invalid startTime/endTime"})", "application/json");
		return;
	}

	const auto start = std::chrono::system_clock::from_time_t(startTimeValue);
	const auto end = std::chrono::system_clock::from_time_t(endTimeValue);

	const std::vector<int> lotIds = m_service.findAvailableReservedLots(start, end);

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"lotIds", lotIds}
	};

	res.set_content(reply.dump(2), "application/json");
}

void Server::handleApiAvailableNormal(const httplib::Request& req, httplib::Response& res)
{
	const int lotId = getLotIdParam(req);
	const auto availability = m_service.getAvailableNormal(lotId);

	nlohmann::json lots = nlohmann::json::array();
	for (const auto& lotEntry : availability)
	{
		nlohmann::json spaces = nlohmann::json::array();
		for (const auto& spaceEntry : lotEntry.second)
		{
			spaces.push_back(
				{
					{"spaceId", spaceEntry.first},
					{"available", spaceEntry.second}
				});
		}

		lots.push_back(
			{
				{"lotId", lotEntry.first},
				{"spaces", spaces}
			});
	}

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"lots", lots}
	};

	res.set_content(reply.dump(2), "application/json");
}

void Server::handleApiAvailableDisabled(const httplib::Request& req, httplib::Response& res)
{
	const int lotId = getLotIdParam(req);
	const auto availability = m_service.getAvailableDisabled(lotId);

	nlohmann::json lots = nlohmann::json::array();
	for (const auto& lotEntry : availability)
	{
		nlohmann::json spaces = nlohmann::json::array();
		for (const auto& spaceEntry : lotEntry.second)
		{
			spaces.push_back(
				{
					{"spaceId", spaceEntry.first},
					{"available", spaceEntry.second}
				});
		}

		lots.push_back(
			{
				{"lotId", lotEntry.first},
				{"spaces", spaces}
			});
	}

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"lots", lots}
	};

	res.set_content(reply.dump(2), "application/json");
}

void Server::handleApiCreateBooking(const httplib::Request& req, httplib::Response& res)
{
	std::string token = getTokenFromCookie(req);
	std::string email = getUsernameForToken(token);
	if (token.empty() || email.empty())
	{
		res.status = 401;
		res.set_content(R"({"error":"unauthenticated"})", "application/json");
		return;
	}

	try
	{
		auto j = nlohmann::json::parse(req.body);

		const std::string registration = j.value("registration", std::string());
		const int lotId = j.value("lotId", 0);
		const std::time_t startTimeValue = static_cast<std::time_t>(j.value("startTime", 0LL));
		const std::time_t endTimeValue = static_cast<std::time_t>(j.value("endTime", 0LL));

		if (registration.empty() || lotId <= 0 || startTimeValue == 0 || endTimeValue == 0)
		{
			res.status = 400;
			res.set_content(R"({"error":"missing or invalid booking fields"})", "application/json");
			return;
		}

		const auto start = std::chrono::system_clock::from_time_t(startTimeValue);
		const auto end = std::chrono::system_clock::from_time_t(endTimeValue);

		if (!m_service.createBooking(email, registration, start, end, lotId))
		{
			res.status = 409;
			res.set_content(R"({"error":"booking could not be created"})", "application/json");
			return;
		}

		broadcastAvailabilityUpdate();

		nlohmann::json reply =
		{
			{"status", "ok"},
			{"lotId", lotId},
			{"email", email},
			{"registration", registration}
		};

		res.set_content(reply.dump(2), "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiGetBookings(const httplib::Request& req, httplib::Response& res)
{
	std::string token = getTokenFromCookie(req);
	std::string email = getUsernameForToken(token);
	if (token.empty() || email.empty())
	{
		res.status = 401;
		res.set_content(R"({"error":"unauthenticated"})", "application/json");
		return;
	}

	const std::vector<TempBooking> bookings = m_service.getUpcomingBookings(email);
	nlohmann::json items = nlohmann::json::array();

	for (const auto& booking : bookings)
	{
		items.push_back(
			{
				{"lotId", booking.lotId},
				{"email", booking.m_email},
				{"registration", booking.m_registration},
				{"startTime", std::chrono::system_clock::to_time_t(booking.m_start)},
				{"endTime", std::chrono::system_clock::to_time_t(booking.m_end)}
			});
	}

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"bookings", items}
	};

	res.set_content(reply.dump(2), "application/json");
}

void Server::handleApiCancelBooking(const httplib::Request& req, httplib::Response& res)
{
	std::string token = getTokenFromCookie(req);
	std::string email = getUsernameForToken(token);
	if (token.empty() || email.empty())
	{
		res.status = 401;
		res.set_content(R"({"error":"unauthenticated"})", "application/json");
		return;
	}

	try
	{
		auto j = nlohmann::json::parse(req.body);

		const int lotId = j.value("lotId", 0);
		const std::string registration = j.value("registration", std::string());
		const std::time_t startTimeValue = static_cast<std::time_t>(j.value("startTime", 0LL));
		const std::time_t endTimeValue = static_cast<std::time_t>(j.value("endTime", 0LL));

		if (lotId <= 0 || registration.empty() || startTimeValue == 0 || endTimeValue == 0)
		{
			res.status = 400;
			res.set_content(R"({"error":"missing or invalid booking fields"})", "application/json");
			return;
		}

		const auto start = std::chrono::system_clock::from_time_t(startTimeValue);
		const auto end = std::chrono::system_clock::from_time_t(endTimeValue);

		if (!m_service.cancelBooking(lotId, email, registration, start, end))
		{
			res.status = 409;
			res.set_content(R"({"error":"booking could not be cancelled"})", "application/json");
			return;
		}

		broadcastAvailabilityUpdate();
		res.set_content(R"({"status":"ok"})", "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiUpdateBooking(const httplib::Request& req, httplib::Response& res)
{
	std::string token = getTokenFromCookie(req);
	std::string email = getUsernameForToken(token);
	if (token.empty() || email.empty())
	{
		res.status = 401;
		res.set_content(R"({"error":"unauthenticated"})", "application/json");
		return;
	}

	try
	{
		auto j = nlohmann::json::parse(req.body);

		const int originalLotId = j.value("originalLotId", 0);
		const std::string originalRegistration = j.value("originalRegistration", std::string());
		const std::time_t originalStartTimeValue = static_cast<std::time_t>(j.value("originalStartTime", 0LL));
		const std::time_t originalEndTimeValue = static_cast<std::time_t>(j.value("originalEndTime", 0LL));

		const int newLotId = j.value("lotId", 0);
		const std::string newRegistration = j.value("registration", std::string());
		const std::time_t newStartTimeValue = static_cast<std::time_t>(j.value("startTime", 0LL));
		const std::time_t newEndTimeValue = static_cast<std::time_t>(j.value("endTime", 0LL));

		if (originalLotId <= 0 || originalRegistration.empty() ||
			originalStartTimeValue == 0 || originalEndTimeValue == 0 ||
			newLotId <= 0 || newRegistration.empty() || newStartTimeValue == 0 || newEndTimeValue == 0)
		{
			res.status = 400;
			res.set_content(R"({"error":"missing or invalid booking fields"})", "application/json");
			return;
		}

		const auto originalStart = std::chrono::system_clock::from_time_t(originalStartTimeValue);
		const auto originalEnd = std::chrono::system_clock::from_time_t(originalEndTimeValue);
		const auto newStart = std::chrono::system_clock::from_time_t(newStartTimeValue);
		const auto newEnd = std::chrono::system_clock::from_time_t(newEndTimeValue);

		if (!m_service.updateBooking(
			email,
			originalLotId,
			originalRegistration,
			originalStart,
			originalEnd,
			newLotId,
			newRegistration,
			newStart,
			newEnd))
		{
			res.status = 409;
			res.set_content(R"({"error":"booking could not be updated"})", "application/json");
			return;
		}

		broadcastAvailabilityUpdate();
		res.set_content(R"({"status":"ok"})", "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::createWebsocketRoute() 
{
	WebSocket("/ws", [&](const httplib::Request& req, httplib::ws::WebSocket& ws)
		{
			std::string token = getTokenFromCookie(req);
			if (!token.empty())
			{
				std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);
				if (m_sessions.tokenToUsername.find(token) == m_sessions.tokenToUsername.end()) token.clear();
			}

			{
				std::lock_guard<std::mutex> lock(m_sessions.websocketMutex);
				m_sessions.websockets.insert(&ws);
				m_sessions.websocketToToken[&ws] = token;
			}

			if (!token.empty())
			{
				sendAvailabilityUpdate(ws);
			}

			std::string msg;
			while (ws.is_open())
			{
				auto res = ws.read(msg);
				if (res == httplib::ws::ReadResult::Fail) break;
				if (res == httplib::ws::ReadResult::Text)
				{
					handleWebsocketMessage(ws, msg);
				}
			}

			cleanupOnClose(ws);
		});
}

void Server::handleWebsocketMessage(httplib::ws::WebSocket& ws, const std::string& msg)
{
	try
	{
		auto parsed = nlohmann::json::parse(msg);
		auto type = parsed.value("type", std::string());

		if (type == "login")
		{
			std::string user = parsed.value("username", std::string());
			std::string pass = parsed.value("password", std::string());

			if (validateAccount(user, pass))
			{
				std::string newToken = createToken();
				{
					std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);
					auto oldToken = m_sessions.usernameToToken.find(user);
					if (oldToken != m_sessions.usernameToToken.end())
					{
						m_sessions.tokenToUsername.erase(oldToken->second);
					}

					m_sessions.tokenToUsername[newToken] = user;
					m_sessions.usernameToToken[user] = newToken;
				}

				{
					std::lock_guard<std::mutex> lock2(m_sessions.websocketMutex);
					m_sessions.websocketToToken[&ws] = newToken;
				}

				nlohmann::json reply =
				{
					{"type", "login"},
					{"status", "ok"},
					{"token", newToken},
					{"user", user}
				};

				ws.send(reply.dump());
				sendAvailabilityUpdate(ws);
			}
			else
			{
				nlohmann::json reply =
				{
					{"type", "login"},
					{"status", "error"},
					{"error", "invalid credentials"}
				};

				ws.send(reply.dump());
			}
		}
	}
	catch (...)
	{
		std::cout << "Error handling websocket message. Ignoring." << std::endl;
	}
}

void Server::cleanupOnClose(httplib::ws::WebSocket& ws)
{
	std::string associatedToken;

	{
		std::lock_guard<std::mutex> lock(m_sessions.websocketMutex);
		m_sessions.websockets.erase(&ws);

		auto websocketIt = m_sessions.websocketToToken.find(&ws);
		if (websocketIt != m_sessions.websocketToToken.end())
		{
			associatedToken = websocketIt->second;
			m_sessions.websocketToToken.erase(websocketIt);
		}
	}

	if (!associatedToken.empty())
	{
		std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);

		auto tokenIt = m_sessions.tokenToUsername.find(associatedToken);
		if (tokenIt != m_sessions.tokenToUsername.end())
		{
			const std::string user = tokenIt->second;
			m_sessions.tokenToUsername.erase(tokenIt);

			auto userIt = m_sessions.usernameToToken.find(user);
			if (userIt != m_sessions.usernameToToken.end() && userIt->second == associatedToken)
			{
				m_sessions.usernameToToken.erase(userIt);
			}
		}
	}
}

void Server::sendAvailabilityUpdate(httplib::ws::WebSocket& ws)
{
	try
	{
		ws.send(buildAvailabilityMessage(m_service).dump());
	}
	catch (...)
	{
		std::cout << "Failed to send availability update." << std::endl;
	}
}

void Server::broadcastAvailabilityUpdate()
{
	std::vector<httplib::ws::WebSocket*> sockets;

	{
		std::lock_guard<std::mutex> lock(m_sessions.websocketMutex);

		for (const auto& [ws, token] : m_sessions.websocketToToken)
		{
			if (ws != nullptr)
			{
				sockets.push_back(ws);
			}
		}
	}

	const std::string payload = buildAvailabilityMessage(m_service).dump();

	for (auto* ws : sockets)
	{
		if (!ws->is_open()) continue;

		try
		{
			ws->send(payload);
		}
		catch (...)
		{
			std::cout << "Failed to broadcast availability update." << std::endl;
		}
	}
}

void Server::startWebsocketBroadcaster()
{
	if (m_broadcasterThread.thread.joinable()) return;

	m_broadcasterThread.thread = std::thread([this]()
	{
		while (m_broadcasterThread.running.load())
		{
			broadcastAvailabilityUpdate();

			std::unique_lock<std::mutex> lock(m_broadcasterThread.mutex);
			m_broadcasterThread.cv.wait_for(
				lock,
				std::chrono::seconds(1),
				[this]() { return !m_broadcasterThread.running.load(); });
		}
	});
}

void Server::start() 
{
	if (m_broadcasterThread.running.exchange(true)) return;
	try 
	{
		std::cout << "Server starting at https://" << m_address << ":" << m_port << "\nPress Ctrl+C to stop and quit." << std::endl;
		startWebsocketBroadcaster();
		if (!listen(m_address, m_port)) 
		{
			m_broadcasterThread.running.store(false);
			m_broadcasterThread.cv.notify_all();
			if (m_broadcasterThread.thread.joinable())
			{
				m_broadcasterThread.thread.join();
			}
			throw std::runtime_error("Server failed to start.");
		}
	}
	catch (const std::exception& e) 
	{
		std::cout << "Runtime error. " << e.what() << " Server not running." << std::endl;
		m_broadcasterThread.running.store(false);
		m_broadcasterThread.cv.notify_all();
		if (m_broadcasterThread.thread.joinable())
		{
			m_broadcasterThread.thread.join();
		}
		throw;
	}
	catch (...) 
	{
		std::cout << "Unexpected error. Server not running." << std::endl;
		m_broadcasterThread.running.store(false);
		m_broadcasterThread.cv.notify_all();
		if (m_broadcasterThread.thread.joinable())
		{
			m_broadcasterThread.thread.join();
		}
		throw;
	}

	m_broadcasterThread.running.store(false);
	m_broadcasterThread.cv.notify_all();
	if (m_broadcasterThread.thread.joinable())
	{
		m_broadcasterThread.thread.join();
	}
}

void Server::stop() 
{
	if (!m_broadcasterThread.running.exchange(false)) return;

	m_broadcasterThread.cv.notify_all();

	std::vector<httplib::ws::WebSocket*> websocketsToClose;
	{
		std::lock_guard<std::mutex> lock(m_sessions.websocketMutex);
		websocketsToClose.reserve(m_sessions.websockets.size());

		for (httplib::ws::WebSocket* ws : m_sessions.websockets)
		{
			if (ws != nullptr)
			{
				websocketsToClose.push_back(ws);
			}
		}
	}

	for (httplib::ws::WebSocket* ws : websocketsToClose)
	{
		try
		{
			ws->close(httplib::ws::CloseStatus::GoingAway, "Server shutting down");
		}
		catch (...)
		{
		}
	}

	httplib::SSLServer::stop();
}
