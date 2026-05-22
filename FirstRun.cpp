#include "Server.h"
#include "Simulation.h"
#include <iostream>
#include <vector>
#include <string>
#include <exception>


Database g_db("localhost", "5432", "PMS", "postgres", "data");
CarParkService g_service(g_db);
Server g_server(g_service, "./cert/cert.crt", "./cert/cert.key");
Simulation g_sim(g_server,g_service, g_db);

// Console control handler to gracefully stop server and broadcaster on Ctrl+C / close
static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
	switch (dwCtrlType) {
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		std::cout << "Shutting down..." << std::endl;
		g_sim.stop();
		g_server.stop();
		g_service.stop();
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	try
	{
		// register console control handler
		SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
		g_sim.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Server failed to start: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}