#pragma warning(disable:4786 4503)

#include <string>
#include <sstream>
#include <fstream>
#include "txtlistmap.h"
#include "contextpattern.h"

#pragma warning(disable:4786 4503)

const StringSet *TxtListMap::getTxtList (const std::string &typeName) const {
	TLMKernel::const_iterator i = theMap.find (typeName);
	if (i == theMap.end ()) {
		return NULL;
	} else {
		return i->second;
	}
}

TxtListMap::~TxtListMap () {
	for (TLMKernel::iterator i = theMap.begin (); i != theMap.end (); ++i) {
		delete i->second;
	}
}

void TxtListMap::addTxtList (const std::string &typeName, const std::string &fileName) {
	if (getTxtList (typeName) != NULL)
		throw CPError ("Multideclared #list name: " + typeName + ".");

	std::ifstream listFile (fileName.c_str ());
	if (!listFile.is_open ()) {
		throw CPError ("Unable to open list file " + fileName + " for #" + typeName + ".");
	}

	StringSet *tl = new StringSet;	// destructor deletes
	theMap.insert (TLMKernel::value_type (typeName, tl));

	std::string txt;
	while (!listFile.eof ()) {
		readNextTxt (listFile, txt);
		insertTxt (txt, *tl);
	}
	listFile.close ();
}
