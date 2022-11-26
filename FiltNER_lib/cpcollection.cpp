#pragma warning(disable:4786 4503)

#include <fstream>
#include "cpcollection.h"
#include "tokenizedtext.h"

#pragma warning(disable:4786 4503)

CPCollection::CPCollection (const std::string &iCpFileName) {
	std::ifstream cpFile (iCpFileName.c_str ());
	if (!cpFile.is_open ()) {
		throw CPError ("Unable to open ContextPattern file " + iCpFileName + ".");
	}
	std::string s, s2;
	cpFile >> s;
	if (s != "%LISTS")
		throw CPError ("Invalid ContextPattern file: %LISTS expected.");
	if (cpFile.eof ())
		throw CPError ("Invalid ContextPattern file: missing %PATTERNS.");

	cpFile >> s;
	while (!cpFile.eof () && s != "%PATTERNS") {
		cpFile >> s2;
		if (s2 == "%PATTERNS")
			throw CPError ("Invalid ContextPattern file: missing filename for last #list.");
		if (cpFile.eof ())
			throw CPError ("Invalid ContextPattern file: missing %PATTERNS.");

		txtLists.addTxtList (s, s2);
		
		cpFile >> s;
	}

	if (cpFile.eof ())
		throw CPError ("Invalid ContextPattern file: %PATTERNS missing or empty.");
	
	while (!cpFile.eof ()) {
		std::getline (cpFile, s);
		if (!s.empty () && s[0] != ';')
			addCPStr (s);
	}
	cpFile.close ();	
}

CPCollection::~CPCollection () {
	for (CPCKernel::iterator i = contextPatterns.begin (); i != contextPatterns.end (); ++i) {
		for (CPPtrList::iterator j = i->second.begin (); j != i->second.end (); ++j) {
			delete *j;
		}
	}

}

void CPCollection::addCPStr (const std::string &src) {
	ContextPattern *cp = new ContextPattern (src, txtLists);
	const std::string &symbName = cp->getFirstSymbName ();
	CPCKernel::iterator i = contextPatterns.find (symbName);
	
	if (i == contextPatterns.end ()) {	// first CP with this symbName
		CPPtrList cppl;
		cppl.push_back (cp);
		contextPatterns.insert (CPCKernel::value_type (symbName, cppl));
	} else {
		i->second.push_back (cp);
	}
}

void CPCollection::process (TokenizedText &tt) {
	for (std::list<Symbol *>::const_iterator symbIt = tt.getSymbols ().begin (); symbIt != tt.getSymbols ().end (); ++symbIt) {
		// look for the CPs with matching first symbol names
		CPCKernel::const_iterator cpmapIt = contextPatterns.find ((*symbIt)->getName ());
		if (cpmapIt != contextPatterns.end ()) {
			for (CPPtrList::const_iterator cpIt = cpmapIt->second.begin (); cpIt != cpmapIt->second.end (); ++cpIt) {
				(*cpIt)->matchAndApply (**symbIt, tt);
			}
		}
	}
}
