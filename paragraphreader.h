#ifndef PARAGRAPHREADER_H
#define PARAGRAPHREADER_H

#include <string>
#include <istream>

class ParagraphReader {
private:
		bool			empty;
		std::istream	&src;
		std::string		buffer;
public:
		ParagraphReader (std::istream &);
		bool		noMore ()		{ return empty; }
		void		readNext (std::string &);
};

#endif /* PARAGRAPHREADER_H */