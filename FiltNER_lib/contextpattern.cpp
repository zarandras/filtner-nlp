#pragma warning(disable:4786 4503)

#include <string>
#include <list>
#include <sstream>
#include "contextpattern.h"
#include "tokenizedtext.h"

#pragma warning(disable:4786 4503)

// ContextPattern

ContextPattern::ContextPattern (const std::string &source, const TxtListMap &txtLists) {

	bodyLen = varsLen =0;

	CPParser parser(source, *this, txtLists);
	parser.parse ();		// elements created and arrays filled by parser; throws CPError
}

ContextPattern::~ContextPattern () {
	for (int i = 0; i < bodyLen; ++i) {
		delete body[i];
	}
}

void ContextPattern::addBodyElem (CPBElem *bElem, const std::string &srcForError) { // called by CPParser
	if (bodyLen < MAX_BELEMS) {
		if (typeid (CPBEVar) == (typeid (*bElem))) {
			if (varsLen < MAX_VARS) {
				vars[varsLen++] = bodyLen;
			} else {
				delete bElem;			// bElem is immediately deleted
				throw CPValidityErrorTooManyVars (srcForError, *this);
			}
		}
		body[bodyLen++] = bElem; // bElem will be deleted by destructor
	} else {
		delete bElem;			// bElem is immediately deleted
		throw CPValidityErrorTooManyBEs (srcForError, *this);
	}
}

ContextPattern::operator std::string () {
	int i;
	std::string s;
	
	if (bodyLen > 0) {
		if (body[0] == NULL)
			s.append ("(nullPtr)");
		else
			s.append (*body[0]);
	} else {
		s.append ("<>");
	}

	for (i = 1; i < bodyLen; ++i) {
		s.append (" + ");
		if (body[i] == NULL)
			s.append ("(nullPtr)");
		else
			s.append (*body[i]);
	}

	return s;
}

CPBEVar	*ContextPattern::getVarByName (const std::string &varName) {			// returns NULL if not declared
	int i = 0;
	while (i < varsLen && (dynamic_cast<CPBEVar *> (body[vars[i]]))->getVarName () != varName) {
		++i;
	}
	if (i == varsLen)
		return NULL; // not found
	else
		return dynamic_cast<CPBEVar *> (body[vars[i]]);
}

CPBEVar &ContextPattern::getFirstCPBEVar () {
	if (varsLen == 0) {
		std::string s (*this);
		throw FiltNERError ("ContextPattern integrity error: no symbol variables: " + s);
	}

	return *(dynamic_cast<CPBEVar *> (body[vars[0]]));
}

const std::string &ContextPattern::getFirstSymbName () {
	return (getFirstCPBEVar ()).getSymbName ();
}

bool ContextPattern::matchAndApply (Symbol &symb, TokenizedText &tt) {
	CPBEVar &firstBEVar = getFirstCPBEVar ();
	int beIdx = vars[0];
	if (!firstBEVar.matchSymb (symb, tt))
		return false;
	std::list<Symbol *> matchingSymbs;
	matchingSymbs.push_back (&symb);
	return (matchAndApplyPrefix (beIdx - 1, tt, symb.getFirstTokenIdx () - 1) && 
			matchAndApplySuffix (beIdx + 1, tt, symb.getLastTokenIdx () + 1, matchingSymbs));
}

bool ContextPattern::matchAndApplyPrefix (int beIdx, TokenizedText &tt, int tokenIdx) {
	if (beIdx < 0) {
		return true;
	} else {
		return body[beIdx]->matchAndApplyCPPrefix (*this, beIdx, tt, tokenIdx);
	}
}

bool ContextPattern::matchAndApplySuffix (int beIdx, TokenizedText &tt, int tokenIdx, std::list<Symbol *> &matchingSymbs) {
	if (beIdx >= bodyLen) {
		// apply operators
		int i;
		std::list<Symbol *>::const_iterator j;
		for (i = vars[0], j = matchingSymbs.begin (); i < varsLen && j != matchingSymbs.end (); i++, j++) {
			(dynamic_cast <CPBEVar *> (body[vars[i]]))->applyOp (**j);
		}
		return true;
	} else {
		return body[beIdx]->matchAndApplyCPSuffix (*this, beIdx, tt, tokenIdx, matchingSymbs);
	}
}

// CPBE... matching

bool CPBENonvar::matchAndApplyCPPrefix (ContextPattern &cp, int beIdx, TokenizedText &tt, int tokenIdx) {
	int i = matchFromBack (tt, tokenIdx);
	if (i > 0) {
		return cp.matchAndApplyPrefix (beIdx - 1, tt, i - 1);
	} else {
		return false;
	}
}

bool CPBENonvar::matchAndApplyCPSuffix (ContextPattern &cp, int beIdx, TokenizedText &tt, int tokenIdx, std::list <Symbol *> &matchingSymbs) {
	int i = matchFromFront (tt, tokenIdx);
	if (i > 0) {
		return cp.matchAndApplySuffix (beIdx + 1, tt, i + 1, matchingSymbs);
	} else {
		return false;
	}
}

int CPBENonvar::matchFromFront (TokenizedText &tt, int tokenIdx) {
	int n = getNrOfTokenPatterns ();
	if (n == 0)
		return tokenIdx;

	// first word - cut punctuation if needed
	std::string s = tt.getTokens ()[tokenIdx].str ();
	if (!matchesTokenPattern (s, 0) && !matchesCutFirstPunct (s, 0)) {
		return -1;
	}

	// words except first and last one
	for (int i = 1; i < n - 1; ++i) {
		if (!matchesTokenPattern (tt.getTokens ()[tokenIdx + i].str (), i))
			return -1;
	}

	// last word - with morphology if needed
	Token &t = tt.getTokens ()[tokenIdx + n - 1];
	if (morphArgs == NULL) {
		s = t.str ();
		if (!matchesTokenPattern (s, n - 1) && !matchesCutFirstPunct (s, n - 1))
			return -1;
	} else {	// morphological mode
		if (!matchMorphology (t, n - 1))
			return -1;
	}

	return tokenIdx + n - 1;
}

int CPBENonvar::matchFromBack (TokenizedText &tt, int tokenIdx) {
	int n = getNrOfTokenPatterns ();
	if (n == 0)
		return tokenIdx;

	// last word - with morphology if needed
	Token &t = tt.getTokens ()[tokenIdx];
	std::string s;
	if (morphArgs == NULL) {
		s = t.str ();
		if (!matchesTokenPattern (s, n - 1) && !matchesCutFirstPunct (s, n - 1))
			return -1;
	} else {	// morphological mode
		if (!matchMorphology (t, n - 1))
			return -1;
	}

	// words except first and last one
	for (int i = 1; i < n - 1; ++i) {
		if (!matchesTokenPattern (tt.getTokens ()[tokenIdx - i].str (), i))
			return -1;
	}

	// first word - cut punctuation if needed
	s = tt.getTokens ()[tokenIdx - n + 1].str ();
	if (!matchesTokenPattern (s, 0) && !matchesCutFirstPunct (s, 0)) {
		return -1;
	}

	return tokenIdx - n + 1;
}

bool CPBENonvar::matchMorphology (Token &t, int tokenPatIdx) {
	if (t.hasKnownMorph ()) {
		char c = t.str ()[t.str ().size () - 1]; // last char of original token - if it's a punctuation, try adding it to the stem

		StringList matchingStems = t.getMatchingMorphStems (*morphArgs);		// check for possible stems with given morphological constraints (morphArgs)
		
		for (StringList::iterator i = matchingStems.begin (); i != matchingStems.end (); ++i) {
			if (matchesTokenPattern (*i, tokenPatIdx) ||
				!(Token::isLastPunct (c) && matchesTokenPattern (i->insert (i->size () - 1, &c), tokenPatIdx))
			   )
				return true;	// a matching stem found
		}
		return false;	// no matching stems found
	} else { // unknown morphology
		std::string s = t.str ();
		return (matchesTokenPattern (s, tokenPatIdx) || matchesCutFirstPunct (s, tokenPatIdx));
	}
}

bool CPBENonvar::matchesCutFirstPunct (std:: string &s, int pos) {
	if (s.empty() || !Token::isFirstPunct (s[0]))
		return false;
	s.erase (s.begin ());
	return (matchesTokenPattern (s, pos));
}

bool CPBENonvar::matchesCutLastPunct (std:: string &s, int pos) {
	if (s.empty() || !Token::isLastPunct (s[s.size () - 1]))
		return false;
	s.erase (s.size () - 1, 1);
	return (matchesTokenPattern (s, pos));
}



bool CPBESTxt::matchesTokenPattern (const std::string &tokenStr, int tokenPatIdx) {
	if (tokenPatIdx < 0 || tokenPatIdx >= surfTokens.size ())
		return false;

	if (patType == Pat_Suffix && tokenPatIdx == 0) {
		std::string s = tokenStr;
		return (s.erase (0, s.size () - surfTokens[tokenPatIdx].size ()) == surfTokens[tokenPatIdx]);	// truncate s at the front
	} else if (patType == Pat_Prefix && tokenPatIdx == getNrOfTokenPatterns () - 1) {
		std::string s = tokenStr;
		return (s.erase (surfTokens[tokenPatIdx].size ()) == surfTokens[tokenPatIdx]);	// truncate s at the back
	} else {
		return (tokenStr == surfTokens[tokenPatIdx]);
	}
}

bool CPBESLst::matchesTokenPattern (const std::string &tokenStr, int tokenPatIdx) {
	if (tokenPatIdx != 0)
		return false;

	StringSet::const_iterator i = txtList.find (tokenStr);
	return (i != txtList.end ());
}


bool CPBEVar::matchAndApplyCPPrefix (ContextPattern &, int beIdx, TokenizedText &, int tokenIdx) {
	// this function cannot be called because matching starts at first CPBEVar
	throw FiltNERError ("Integrity error: First variable in ContextPattern is not the one ought to be.");
}

bool CPBEVar::matchAndApplyCPSuffix (ContextPattern &cp, int beIdx, TokenizedText &tt, int tokenIdx, std::list <Symbol *> &matchingSymbs) {
	bool match = false;
	const std::set<Symbol *> &symbolsHere = tt.getTokens ()[tokenIdx].getParentSymbols ();

	for (std::set<Symbol *>::const_iterator i = symbolsHere.begin ();  i != symbolsHere.end (); ++i) {
		if ((*i)->getFirstTokenIdx () == tokenIdx && matchSymb (**i, tt)) {
			matchingSymbs.push_back (*i);
			match = match || cp.matchAndApplySuffix (beIdx + 1, tt, (*i)->getLastTokenIdx () + 1, matchingSymbs);
			matchingSymbs.pop_back ();
		}
	}
	return match;
}


bool CPBEVar::matchSymb (Symbol &symb, TokenizedText &tt) {
	// check name
	if (getSymbName () != symb.getName ())
		return false;
	
	// check arguments
	if (symbArgs != NULL) {
	  for (SymbArgs::const_iterator i = symbArgs->begin (); i != symbArgs->end (); ++i) {
		const std::string *s = symb.getArgVal (i->first);
		if (s == NULL) {
			throw FiltNERError ("Symbol " + symb.getName () + " does not have the argument " + i->first + " defined in ContextPatterns.\n");
		}
		if (*s != i->second)
			return false;
	  }
	}
	
	// check pattern	
	switch (patType) {
		case Pat_Prefix:
			return (patStrElem->matchFromFront (tt, symb.getFirstTokenIdx ()) > 0);

		case Pat_Suffix:
			return (patStrElem->matchFromBack (tt, symb.getLastTokenIdx ()) > 0);

		default: /* Pat_None */
			return true;
	}
}

// CPBE... other


void CPBENonvar::setMorphArgs (StringList *sl) {
	if (morphArgs != NULL)
		throw FiltNERError ("Program error: CPBENonvar::setMorphArgs called more than once.");
			// can be called only once

	morphArgs = sl;	// destructor deletes
}

CPBESTxt::CPBESTxt (const std::string iSurf, TPatType iPatType) : patType (iPatType) {
	std::istringstream surfStream (iSurf);
	std::string s;
	while (!surfStream.eof ()) {
		surfStream >> s;
		surfTokens.push_back (s);
	}
	switch (patType) {
		case Pat_Prefix: {
				std::string &s = surfTokens[surfTokens.size () - 1];
				s.erase (s.size () - 1, 1);	// erase last char ('*')
			}
			break;

		case Pat_Suffix:
			surfTokens[0].erase (0, 1); // erase first char ('*')
			break;

		default:
			break;
	}
}

CPBENonvar::operator std::string () {
	std::string s = strPrefix ();

	if (morphArgs != NULL) {
		s.append (" [");

		StringList::const_iterator i = morphArgs->begin ();
		if (i != morphArgs->end ())
			s.append (*i);
		i++;
		while (i != morphArgs->end ()) {
			s.append (", " + *i);
		}
		s.append ("]");
	}
	return s;
}

std::string CPBESLst::strPrefix () {
	return "#<" + ((txtList.empty ()) ? ">": *txtList.begin () + " ... >");
}

std::string CPBESTxt::strPrefix () {
	std::string s = "\"";
	for (int i = 0; i < surfTokens.size (); ++i) {
		s.append (surfTokens[i] + ((i == surfTokens.size () - 1) ? "" : " "));
	}
	s.append ("\"");
	return s;
}

CPBEVar::operator std::string () {
	std::string s = varName;

	switch (op) {
		case Op_Kill:
			s.append ("!");
			break;

		case Op_Unsure:
			s.append ("?");
			break;

		case Op_Sure:
			s.append (".");
			break;
	}

	switch (patType) {
		case Pat_Prefix:
			s.append (" {");
			if (patStrElem == NULL) {
				s.append("?");
			} else {
				s.append (*patStrElem);
			}
			s.append (" *}");
			break;

		case Pat_Suffix:
			s.append (" {* ");
			if (patStrElem == NULL) {
				s.append("?");
			} else {
				s.append (*patStrElem);
			}
			s.append ("}");
			break;

		default:
			break;
	}

	s.append (" : " + symbName);

		if (symbArgs != NULL) {
			s.append (" (");
			
		SymbArgs::const_iterator i = symbArgs->begin ();
		if (i != symbArgs->end ())
			s.append (i->first + " = \"" + i->second + "\"");
		i++;
		while (i != symbArgs->end ()) {
			s.append (", " + i->first + " = \"" + i->second + "\"");
			i++;
		}
		s.append (")");
	}
	return s;	
}

void CPBEVar::applyOp (Symbol &symb) {
	switch (op) {
		case Op_Kill:
			symb.kill ();
			break;

		case Op_Unsure:
			symb.decCertLev ();
			break;

		case Op_Sure:
			symb.incCertLev ();
			break;

		default:
			break;
	}
}
