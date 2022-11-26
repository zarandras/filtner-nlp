#pragma warning(disable:4786 4503)

#include "symbol.h"

#pragma warning(disable:4786 4503)

const std::string	*Symbol::getArgVal (const std::string &argName) const {
	SymbArgs::const_iterator i = args.find (argName);
	if (i == args.end ())
		return NULL;
	else
		return &(i->second);
}

bool Symbol::deleteArg (const std::string &argName) {
	return (args.erase (argName) > 0);
}

void Symbol::transfUnsureArg (const std::string &unsureArgName) {
  if (!unsureArgName.empty ()) {	// /U command line argument specified
	const std::string *unsure = getArgVal (unsureArgName);
	if (unsure != NULL && *unsure == "YES") {
		deleteArg (unsureArgName);
		setCertLev (SYMB_UNSURE);
	}
  }
}

void Symbol::writeArgs (std::ostream &os) const {
	for (SymbArgs::const_iterator j = args.begin (); j != args.end (); ++j) {
		os << " " << j->first << "=" << j->second;
	}
}
