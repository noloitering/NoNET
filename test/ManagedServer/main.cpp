#include "../../src/Server.h"

int main(int argc, char ** argv)
{
	enet_initialize();
	std::shared_ptr< NoNET::ManagedServer > server = std::make_shared< NoNET::ManagedServer >();
	NoNET::startup(server->getAddress(), *(server));
	
	while (true)
	{
		server->update();
		server->poll(1000);
	}
	enet_deinitialize();
	
	return 0;
}