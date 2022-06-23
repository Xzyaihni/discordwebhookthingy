#include <cstdlib>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "hookctl.h"


void help_message(const char* exec_path)
{
	std::cout << "usage: " << exec_path << " [args] /path/to/webhook\n\n";
	std::cout << "args:\n";
	std::cout << "	-m		message to send\n";
	std::cout << "	-n		name of the sender (default is the webhook's name)\n";
	std::cout << std::endl;
}

int main(int argc, char* argv[])
{
	std::string send_message = "";
	std::string send_name = "";

	if(argc==1)
	{
		help_message(argv[0]);
		return 4;
	}

	while(true)
	{
		switch(getopt(argc, argv, "hm:n:"))
		{
			case 'n':
				send_name = std::string(optarg);
				continue;

			case 'm':
				send_message = std::string(optarg);
				continue;

			case 'h':
				help_message(argv[0]);
				return 3;

			case -1:
				break;
		}
		break;
	}

	if(send_message=="")
	{
		std::cout << "message argument not set!!" << std::endl;
		help_message(argv[0]);
		return 2;
	}

	if(optind >= argc)
	{
		std::cout << "path to webhook address not given!!" << std::endl;
		help_message(argv[0]);
		return 1;
	}


	const yhook::controller c_ctl(argv[optind]);

	if(send_name!="")
	{
		c_ctl.send_as(send_message, send_name);
	} else
	{
		c_ctl.send_default(send_message);
	}
}