#pragma warning(disable:4786 4503)

#include <string>
#include <list>

#include "contextpattern.h"

#pragma warning(disable:4786 4503)

/* LL(1) parsing --------------------------------------------------------------------------------------------- */

CPParser::CPParser (const std::string &iSource, ContextPattern &iDest, const TxtListMap &iTxtLists) : source(iSource), dest(iDest), txtLists(iTxtLists) {
	; // empty
}

// iterator functions

void CPParser::initIter () {
	i = source.begin ();
	seekIter ();
}

void CPParser::stepIter () {
	if (i == source.end ())
		throw CPParseErrorEoln (source);
	do {
		++i;
	} while (i != source.end () && (*i == ' ' || *i == '\t'));	
}

void CPParser::seekIter () {
	while (i != source.end () && (*i == ' ' || *i == '\t')) {
		i++;
	}
}

char CPParser::atIter () {
	if (i != source.end ()) {
		return *i;
	} else {
		return '\0';
	}
}

// main parse function

void CPParser::parse () throw (CPError) {
	initIter ();
	parseBody ();
	acceptChar ('=');
	acceptChar ('>');
	parseHead ();
}

// terminals

void CPParser::acceptChar (char c) {
	if (atIter () == c) {
		stepIter ();
	} else {
		throw CPParseErrorChar (source, atIter (), c);
	}
}

void CPParser::acceptTxt (std::string &s, TPatType *patType) {
	s = "";
	if (patType != NULL)
		*patType = Pat_None;
	char c = atIter ();
	if (c != '"')
		throw CPParseErrorChar (source, c, '"');
	i++;			// stepIter () is not suitable because it skips whitespaces!
	c = atIter ();
	bool mustEnd = false; // if \* is not the first character, it must be the last one
	while (c !=  '\0' && c != '"') {
		if (mustEnd) {
			throw CPParseErrorIllegalUse (source, '*');
		}
		if (c == '\\') {	// special chars
			i++;
			c = atIter ();
			switch (c) {
				case '\\':
				case '"':
					s.append (char2str (c));	// c stays \ or "
					break;

				case '*':
					if (patType == NULL) {
						throw CPParseErrorIllegalUse (source, c);
					} else if (s.empty ()) {
						*patType = Pat_Suffix;
					} else if (*patType != Pat_None) {
						throw CPParseErrorIllegalUse (source, c);
					} else {
						*patType = Pat_Prefix;
						mustEnd = true;
					}
					s.append (char2str (c));	// put * into the text (to preserve nr. of tokens even if "\*" forms a token)
					break;

				default:
					throw CPParseErrorTxt (source, c);
					break;
			}
		} else {
			s.append (char2str (c));
		}
		i++;
		c = atIter ();
	}
	if (c == '\0')
		throw CPParseErrorTxt (source, '\0');
	stepIter (); // skip "
}

void CPParser::acceptName (std::string &s) {
	s = "";
	char c = atIter ();
	while ((c >= 'a' && c <='z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_')) {
		s.append (char2str (c));
		i++;
		c = atIter ();
	}
	if (s.empty ())
		throw CPParseErrorName (source, c);
	seekIter ();
}

// Head

void CPParser::parseHead () {
	parseHElement ();
	parseHRest ();
}

void CPParser::parseHRest () {
	switch (atIter ()) {
		case ',':
			acceptChar ();
			parseHead ();
			break;

		default: 
			// no error
			break;
	}
}

void CPParser::parseHElement () {
	std::string v;
	CPBEVar::Op op;
	
	acceptName (v);
	parseHEOperator (op);
	
	CPBEVar *ve = dest.getVarByName (v);
	if (ve == NULL)
		throw CPValidityErrorUndeclaredVar (source, dest, v);
	if (ve->getOp () != CPBEVar::Op_None)
		throw CPValidityErrorHeadMultiVar (source, dest, v);

	ve->setOp (op);
}

void CPParser::parseHEOperator (CPBEVar::Op &op) {
	switch (atIter ()) {
		case '.':
			acceptChar ();
			op = CPBEVar::Op_Sure;
			break;

		case '!':
			acceptChar ();
			op = CPBEVar::Op_Kill;
			break;

		case '?':
			acceptChar ();
			op = CPBEVar::Op_Unsure;
			break;

		default:
			throw CPParseErrorChars (source, atIter (), ".!?");
			break;
	}
}

// Body

void CPParser::parseBody () {
	CPBElem *bElem;
	bElem = parseBElement ();
	dest.addBodyElem (bElem, source); // bElem is immeditately deleted on error, otherwise deleted by ~ContextPattern
	parseBRest ();
}

void CPParser::parseBRest () {
	switch (atIter ()) {
		case '+':
			acceptChar ();
			parseBody ();
			break;

		default: 
			// no error
			break;
	}
}

CPBElem *CPParser::parseBElement () {
	switch (atIter ()) {
		case '"':
		case '#':
			return parseBENonvar ();
			break;

		default:	// alphanumeric - if not, acceptName throws an error inside parseBEVar
			return parseBEVar ();
			break;
	}
}

// non-variable

CPBENonvar *CPParser::parseBENonvar () {
	CPBENonvar *s = parseBEStr ();
	try {
		s->setMorphArgs (parseMArgs ());
	} catch (CPError) {
		delete s;  	// must be deleted immediately - deleted by ~CPBELex () or ~ContextPattern () if no error occurs here
		throw;
	}
	return s;
}

CPBENonvar *CPParser::parseBEStr () {
	switch (atIter ()) {
		case '"':
			{
				std::string s;
				TPatType pat = Pat_None;
				acceptTxt (s, &pat);
				return new CPBESTxt (s, pat);
			}
			break;

		case '#':
			{
				std::string s;
				acceptChar ();
				acceptName (s);
				const StringSet *tl = txtLists.getTxtList (s);
				if (tl == NULL) {
					throw CPValidityErrorUndeclaredList (source, dest, s);
				} else {
					return new CPBESLst (*tl);
				}
			}
			break;

		default:
			throw CPParseErrorChars (source, atIter (), "\"#");
			break;
	}
}

StringList *CPParser::parseMArgs () {
	switch (atIter ()) {
		case '[': 
			{
				acceptChar ();
				StringList *mArgs = new StringList;
				try {
					parseMArgList (*mArgs);
					acceptChar (']');
				} catch (CPError) {
					delete mArgs;  	// must be deleted immediately - deleted by ~CPBENonvar () if no error occurs here
					throw;
				}
				return mArgs;
			}
		default:
			return NULL; // no args - not the same as "[]"!
			break;
	}
}

void CPParser::parseMArgList (StringList &mArgs) {
	switch (atIter ()) {
		case ']': /* FOLLOW - default */
			// no error
			break;

		default: // alphanumeric - if not, acceptName throws an error
			{
				std::string s;
				acceptName (s);
				mArgs.push_back (s);
				parseMArgRest (mArgs);
				break;
			}
	}
}

void CPParser::parseMArgRest (StringList &mArgs) {
	switch (atIter ()) {
		case ',':
			acceptChar ();
			parseMArgList (mArgs);
			break;

		default:
			// no error
			break;
	}
}

// variable (symbol)

CPBEVar *CPParser::parseBEVar () {
	std::string var;
	acceptName (var);
	if (dest.getVarByName (var) != NULL) {
		throw CPValidityErrorMultideclaredVar (source, dest, var);
	}

	TPatType patType;
	std::string symb;
	std::map<std::string, std::string> *symbArgs;
	CPBENonvar *patStrElem;

	parseBEVarDef (patType, patStrElem, symb, symbArgs);

	return new CPBEVar (var, symb, symbArgs, patType, patStrElem);
}


void CPParser::parseBEVarDef (TPatType &patType, CPBENonvar *&patStrElem, std::string &symb, SymbArgs *&symbArgs) {
	switch (atIter ()) {
		case '{':
			acceptChar ();
			patStrElem = parseVarPattern (patType);
			try {
				acceptChar ('}');
				acceptChar (':');
				parseSymb (symb, symbArgs);
			} catch (CPError) {
				if (patType != Pat_None) {
					delete patStrElem;  	// must be deleted immediately - deleted by ~CPBEVar () if no error occurs here
				}
				throw;
			}
			break;

		case ':':
			patStrElem = NULL;
			acceptChar ();
			parseSymb (symb, symbArgs);
			break;

		default:
				throw CPParseErrorChars (source, atIter (), "{:");
				break;
	}
}

CPBENonvar *CPParser::parseVarPattern (TPatType &patType) {
	switch (atIter ()) {
		case '*': 
			acceptChar ();
			patType = Pat_Suffix;
			return parseBENonvar ();
			break;

		default: // " or # - if not, parseBEStr throws an error inside parseBENonvar
			patType = Pat_Prefix;
			CPBENonvar *patStrElem = parseBENonvar ();
			try {
				acceptChar ();
				return patStrElem;
			} catch (CPError) {
				delete patStrElem;  	// must be deleted immediately - deleted by ~CPBEVar () if no error occurs here
				throw;
			}
			break;
	}
}

void CPParser::parseSymb (std::string &symb, SymbArgs *&symbArgs) {
	acceptName  (symb);
	symbArgs = parseSArgs ();
}

SymbArgs *CPParser::parseSArgs () {
	switch (atIter ()) {
		case '(':
			{
				acceptChar ();
				SymbArgs *sArgs = new SymbArgs;
				try {
					parseSArgList (*sArgs);
					acceptChar (')');
				} catch (CPError) {
					delete sArgs;  	// must be deleted immediately - deleted by ~CPBEVar () if no error occurs here
					throw;
				}
				return sArgs;
			}
		default:
			return NULL; // no args
			break;
	}
}

void CPParser::parseSArgList (SymbArgs &sArgs) {
	parseSArg (sArgs);
	parseSArgRest (sArgs);
}

void CPParser::parseSArgRest (SymbArgs &sArgs) {
	switch (atIter ()) {
		case ',':
			acceptChar ();
			parseSArgList (sArgs);
			break;

		default:
			// no error
			break;
	}
}

void CPParser::parseSArg (SymbArgs &sArgs) {
	std::string name, value;
	acceptName (name);
	acceptChar ('=');
	parseSArgVal (value);
	sArgs.insert (std::map<std::string, std::string>::value_type (name, value));
}

void CPParser::parseSArgVal (std::string &s) {
	switch (atIter ()) {
		case '"':
			acceptTxt (s, NULL);
			break;

		default:	// alphanumeric - if not, acceptName throws an error
			acceptName (s);	// acceptName accepts numbers as well
			break;
	}
}

std::string char2str (char c) {
	std::string s;
	s.insert (s.end (), c);
	return s;
}
