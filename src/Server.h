#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "ThreadControl.h"
#include "CarParkService.h"
#include <string>
#include <httplib.h>
#include <set>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

// Session management struct.
struct Sessions
{
	std::mutex sessionMutex;
	std::mutex websocketMutex;
	std::unordered_map<std::string, std::string> tokenToUsername;
	std::unordered_map<std::string, std::string> usernameToToken;
	std::unordered_map<httplib::ws::WebSocket*, std::string> websocketToToken;
	std::set<httplib::ws::WebSocket*> websockets;
};

// Main server class, inherits from SSLServer to support HTTPS.
// Contains all route definitions and session management logic.
class Server : public httplib::SSLServer
{
private:
	CarParkService& m_service;
	const std::string m_address;
	const int m_port;
	ThreadControl m_broadcasterThread;
	Sessions m_sessions;

	std::string loadFile(const std::string& path);
	std::string createToken();
	std::string getTokenFromCookie(const httplib::Request& req);
	std::string getUsernameForToken(const std::string& token);
	bool validateAccount(const std::string& email, const std::string& password);
	void createRoutes();
	void createFileRoute(const std::string& route, const std::string& path, const std::string& type);
	void handleApiLogin(const httplib::Request& req, httplib::Response& res);
	void handleApiUser(const httplib::Request& req, httplib::Response& res);
	void handleApiCreateAccount(const httplib::Request& req, httplib::Response& res);
	void handleApiUpdates(const httplib::Request& req, httplib::Response& res);
	void handleApiPredictNormal(const httplib::Request& req, httplib::Response& res);
	void handleApiPredictDisabled(const httplib::Request& req, httplib::Response& res);
	void handleApiAvailableReservedLots(const httplib::Request& req, httplib::Response& res);
	void handleApiAvailableNormal(const httplib::Request& req, httplib::Response& res);
	void handleApiAvailableDisabled(const httplib::Request& req, httplib::Response& res);
	void handleApiCreateBooking(const httplib::Request& req, httplib::Response& res);
	void handleApiGetBookings(const httplib::Request& req, httplib::Response& res);
	void handleApiCancelBooking(const httplib::Request& req, httplib::Response& res);
	void handleApiUpdateBooking(const httplib::Request& req, httplib::Response& res);
	void createWebsocketRoute();
	void startWebsocketBroadcaster();
	void handleWebsocketMessage(httplib::ws::WebSocket& ws, const std::string& msg);
	void cleanupOnClose(httplib::ws::WebSocket& ws);
	void sendAvailabilityUpdate(httplib::ws::WebSocket& ws);
	void broadcastAvailabilityUpdate();
	void sendBookingsUpdate(httplib::ws::WebSocket& ws, const std::string& email);
	void broadcastUserBookingsUpdate(const std::string& email);
	std::string getCookieValue(const httplib::Request& req, const std::string& cookieName);
	bool tryGetAdminEmail(const httplib::Request& req, std::string& email);
	void handleApiAdminLogin(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminUser(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminLotActivity(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminParkedWithoutTicket(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminCurrentBookings(const httplib::Request& req, httplib::Response& res);

public:
	Server(CarParkService& service, const char* cert = "./cert/cert.crt", const char* key = "./cert/cert.key", std::string address = "127.0.0.1", int port = 8080);
	~Server();
	void start();
	void stop();
};

