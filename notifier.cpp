#include <cstdlib>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "hookctl.h"
#include "notifyctl.h"


void help_message(const char* exec_path)
{
	std::cout << "usage: " << exec_path << " [args] /path/to/config\n\n";
	std::cout << "args:\n";
	std::cout << "	-w		webhook url\n";
	std::cout << std::endl;
}

int main(int argc, char* argv[])
{
	std::string hook_url = "";

	if(argc==1)
	{
		help_message(argv[0]);
		return 4;
	}

	while(true)
	{
		switch(getopt(argc, argv, "hw:"))
		{
			case 'w':
				hook_url = std::string(optarg);
				continue;

			case 'h':
				help_message(argv[0]);
				return 3;

			case -1:
				break;
		}
		break;
	}

	if(hook_url=="")
	{
		std::cout << "-w option is mandatory!!" << std::endl;
		help_message(argv[0]);
		return 2;
	}

	if(optind >= argc)
	{
		std::cout << "path to config file not given!!" << std::endl;
		help_message(argv[0]);
		return 1;
	}

	const yhook::controller c_ctl(hook_url);
	ynotif::controller c_notifier(argv[optind]);
	c_notifier.start(c_ctl, true);
}