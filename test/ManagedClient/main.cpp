#include "../../src/Server.h"

int main(int argc, char ** argv)
{
	enet_initialize();
	std::shared_ptr< NoNET::ManagedClient > client = std::make_shared< NoNET::ManagedClient >("test");
	NoNET::startup(NULL, *(client));
	if ( argc > 1 )
	{
		std::cout << "connecting to " << argv[1] << std::endl;
		if ( client->connect(argv[1], NoNET::DEFAULTPORT) == 0 )
		{
			std::cout << "connection succesful" << std::endl;
		}
		else
		{
			std::cerr << "failed to connect to server" << std::endl;
			
			return 1;
		}
	}
	
	InitWindow(800, 450, "client");
	while (!WindowShouldClose())
	{
		client->poll(1000);
		BeginDrawing();
		ClearBackground(BLACK);
		EndDrawing();
	}
	client->disconnect();
	enet_deinitialize();
	CloseWindow();
	
	return 0;
}