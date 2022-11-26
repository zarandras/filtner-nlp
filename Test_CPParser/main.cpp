#pragma warning(disable:4786 4503)

#include <iostream>
#include <string>
#include "contextpattern.h"

#pragma warning(disable:4786 4503)

int main(int argc, char *argv[])
{
	char line[256];
	TxtListMap txtLists;
	do {
		std::cin.getline (line, 256);
		try {
			ContextPattern cp (line, txtLists);
			std::string s = cp;
			std::cout << s << "\n";
		} catch (CPError e) {
			std::string s = e;
			std::cerr << s << "\n";
		}
	} while (line[0] != '\0');
	return 0;
}
