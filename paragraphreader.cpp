#include <string>
#include <istream>

#include "paragraphreader.h"

ParagraphReader::ParagraphReader (std::istream &iSrc) : empty (), src(iSrc), buffer("") {
	while (src.good () && buffer.empty ()) {
		std::getline (src, buffer);
	}
	empty = buffer.empty ();
}

void ParagraphReader::readNext (std::string &paragraph) {
	paragraph = "";
	if (!empty) {
		while (src.good () && !buffer.empty ()) {
			paragraph += buffer;
			std::getline (src, buffer);
		}
		if (!buffer.empty ()) {
			paragraph += buffer;
			buffer = "";
		}
		while (src.good () && buffer.empty ()) {
			std::getline (src, buffer);
		}
		empty = buffer.empty ();
	}
}
