#pragma warning(disable:4786 4503)

#include <iostream>
#include <string>
#include "paragraphreader.h"

#pragma warning(disable:4786 4503)

int main(int argc, char *argv[])
{
	std::string p;
	ParagraphReader reader (std::cin);

	while (!reader.noMore ()) {
		reader.readNext (p);
		std::cout << "\n<p>\n" << p << "\n</p>\n";
		std::cout.flush ();
	}
	return 0;
}

