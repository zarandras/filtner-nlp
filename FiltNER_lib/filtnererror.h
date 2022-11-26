#ifndef FILTNERERROR_H
#define FILTNERERROR_H

class FiltNERError {
	std::string str;
public:
	FiltNERError (std::string iStr) : str (iStr) {}
	operator std::string () const { return str; }
	std::string what () { return str; }
};

#endif 