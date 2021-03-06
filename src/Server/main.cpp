#include "..\Service\pch.hpp" 

#include "Application.hpp"
#include "..\Service\Debugger.hpp"

// _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

int __cdecl main(int argc, char **argv)
{
	try
	{
		Application *app = nullptr;

		if (argc < 2)
		{
			$warn std::cerr << "usage: server [port]" << std::endl;

			app = new Application();
		}
		else
			app = new Application(argv);

		app->run();

		app->close();
		delete app;
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unhandled exception\n";
	}

	system("pause");
	return 0;
}

