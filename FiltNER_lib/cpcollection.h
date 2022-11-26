#ifndef CPCOLLECTION_H
#define CPCOLLECTION_H

#include <string>
#include <list>
#include <map>
#include "contextpattern.h"

class TokenizedText;

class CPCollection {
private:
	typedef std::list<ContextPattern *>			CPPtrList;
	typedef std::map<std::string,  CPPtrList>	CPCKernel;
	TxtListMap	txtLists;
	CPCKernel contextPatterns;	// first: symbol name

	void	addCPStr (const std::string &);	// converts string to CP and inserts
public:
			 CPCollection (const std::string &iCpFileName);
			~CPCollection ();
	void	 process (TokenizedText &);	// finds matches of patterns and applies operators on symbols
};

#endif /* CPCOLLECTION_H */
