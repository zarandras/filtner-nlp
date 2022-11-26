#pragma warning(disable:4786 4503)

#include <iostream>
#include <string>
#include <list>
#include <stdexcept>

#include "tokenizedtext.h"
#include "symbol.h"
#include "filtnererror.h"

#include "morf.h"	// wrapper to Humor

#pragma warning(disable:4786 4503)


Morf *Token::humor = NULL;

const std::string Token::humorEngineFileName = "ml6mor32.dll";
const std::string Token::humorStemmerFileName = "stem.dll";
const std::string Token::humorLexiconFileName = "huau.lex";
const std::string Token::humorLemmaFileName = "hustem.lex";
const int Token::humorLID = 1038;

void Token::initMorphology (const std::string &basePath, int codePage) {
	if (humor != NULL)
		throw FiltNERError ("Program error: Attempt to initialize Humor twice.");

	MorfInit mi;

	mi.base_path = basePath;
	mi.engine = humorEngineFileName;
	mi.stemmer = humorStemmerFileName;
	mi.ana_lexicon = humorLexiconFileName;
	mi.ana_lemfilname = humorLemmaFileName;
	mi.lid = humorLID;
	mi.cp = codePage;

	try {
		humor = new Morf (mi);
	} catch (Morf::bad_morf &e) {
		throw FiltNERError ("Humor error on init: " + e.msg);
	}
}

void Token::doneMorphology () {
	if (humor != NULL)
		delete humor;
}

void Token::fillMorphology () {
	if (morphology != NULL)	// only get once
		return;

	morphology = new std::vector<std::string>;
	if (humor == NULL)	// Humor is not initialized - everything is unknown
		return;

	try {
		humor->getAnas (str (), *morphology);
	} catch (Morf::bad_morf &e) {
		throw FiltNERError ("Humor error on analyze: " + e.msg);
	}
}

void Token::resetMorphology () {
	if (morphology != NULL) {
		delete morphology;
		morphology = NULL;
	}
}

bool Token::hasKnownMorph () {
	fillMorphology ();

	return (!morphology->empty ());
}

StringList Token::getMatchingMorphStems (const StringList &morphArgs) {
	fillMorphology ();

	StringList matchingStems;
	for (int i = 0; i < morphology->size (); ++i) {
		int posInCurrMorphStr = 0;
		std::string morphArgStr;
		StringList::const_iterator mArgsIt = morphArgs.begin ();
		while (posInCurrMorphStr != std::string::npos && mArgsIt != morphArgs.end ()) {
			morphArgStr = "["+ *mArgsIt + "]";
			posInCurrMorphStr = (*morphology)[i].find (morphArgStr, posInCurrMorphStr);	// step to next occurence of next arg
		}
		
		if (posInCurrMorphStr != std::string::npos) {	// all args found in given order
			int k = (*morphology)[i].find ('[');
			if (k == std::string::npos) {
				matchingStems.push_back ((*morphology)[i]);
			} else {
				std::string j = (*morphology)[i];
				matchingStems.push_back (j.erase (k));
			}
		}
	}

	return matchingStems;
}

TokenizedText::~TokenizedText () {
	for (std::list<Symbol *>::iterator i = symbList.begin (); i != symbList.end (); ++i) {
		delete *i;	// created by addSymbol
	}
}
	
void TokenizedText::registerSymbol (Symbol *s) {
	try {
		for (int i = s->getFirstTokenIdx (); i <= s->getLastTokenIdx (); ++i) {
			tokens.at (i).addParentSymbol (s);
		}
	} catch (std::out_of_range &e) {
		std::string s = "HumorESK generated symbols are not consistent with input text tokens (out of range: ";
		throw FiltNERError (s + e.what () + ").");
	}
}

void TokenizedText::unregisterSymbol (Symbol *s) {
	for (int i = s->getFirstTokenIdx (); i <= s->getLastTokenIdx (); ++i) {
		if (!tokens.at (i).delParentSymbol (s))
			throw FiltNERError ("TokenizedText critical integrity error.");
	}
}


void TokenizedText::killOverlappingSymbols () {
	for (std::list<Symbol *>::iterator i = symbList.begin (); i != symbList.end (); ++i) {
		if ((*i)->sure ()) {
			if (containedBySure ((*i)->getFirstTokenIdx (), (*i)->getLastTokenIdx ()))
				(*i)->kill ();
		} else if (!(*i)->killed ()) { // unsure
			if (isSureParentAt ((*i)->getFirstTokenIdx ()) || isSureParentAt ((*i)->getLastTokenIdx ()))
				(*i)->kill ();
		}
	}
}


void TokenizedText::purgeKilledSymbols () {
	for (std::list<Symbol *>::iterator i = symbList.begin (); i != symbList.end (); ++i) {
		while (i != symbList.end () && (*i)->killed ()) {
			delete *i;
			i = symbList.erase (i);
		}
	}
}

void TokenizedText::addSymbol (const Symbol &symb) {
	Symbol *s = new Symbol (symb); // destructor deletes
	symbList.push_back (s);
	registerSymbol (s);	// checks for tokenidx validity
}

std::set<Symbol *>	TokenizedText::sureParentsAt (int idx) {
	std::set<Symbol *> s;
	try {
		const std::set<Symbol *> &parents = tokens.at (idx).getParentSymbols ();
		for (std::set<Symbol *>::const_iterator i = parents.begin (); i != parents.end (); ++i) {
			if ((*i)->sure ()) {
				s.insert (*i);
			}
		}
	} catch (std::out_of_range) {
		// will return empty set s
	}
	return s;
}

bool TokenizedText::containedBySure (int firstIdx, int lastIdx) {
	std::set<Symbol *> sp1, sp2;
	sp1 = sureParentsAt (firstIdx);
	sp2 = sureParentsAt (lastIdx);
	struct Found {};
	try {
		for (std::set<Symbol *>::const_iterator i = sp1.begin (); i != sp1.end (); ++i) {
			for (std::set<Symbol *>::const_iterator j = sp2.begin (); j != sp2.end (); ++j) {
				if ((*i == *j) && ((*i)->getFirstTokenIdx () < firstIdx || (*i)->getLastTokenIdx () > lastIdx)) {
					throw Found ();
				}
			}
		}
	} catch (Found &) {
		return true;
	}
	return false;
}

void TokenizedText::keepMostCertSymbols () {
	// sort by certLev
	{
		greaterSymbLev gsl;
		symbList.sort (gsl);
	}

	// kill symbol if another symbol with higher level found at its first position
	//	(in this case, the current symbol is completely nested into the other)
	// and kill all lower level symbols found at first or last position:
	int sLev;
	bool keepCurr;
	for (std::list<Symbol *>::iterator i = symbList.begin (); i != symbList.end (); ++i) {
		sLev = (*i)->getCertLev ();
		keepCurr = true;
		
		// check at first token:
		const std::set<Symbol *> &currSymbSet1 = tokens[(*i)->getFirstTokenIdx ()].getParentSymbols ();
		for (std::set<Symbol *>::const_iterator j = currSymbSet1.begin (); j != currSymbSet1.end (); ++j) {
			if ((*j)->getCertLev () > sLev) {
				keepCurr = false;
			} else if ((*j)->getCertLev () < sLev) {
				(*j)->kill ();
			}
		}

		// check at last token:
		const std::set<Symbol *> &currSymbSet2 = tokens[(*i)->getLastTokenIdx ()].getParentSymbols ();
		for (std::set<Symbol *>::const_iterator k = currSymbSet2.begin (); k != currSymbSet2.end (); ++k) {
			if ((*k)->getCertLev () < sLev) {
				(*k)->kill ();
			}
		}
	}

	purgeKilledSymbols ();
	// sort by postiton
	{
		greaterSymbPos gsp;
		symbList.sort (gsp);
	}
}

std::string TokenizedText::tokenStr (const Token &t) {
	if (t.str ().empty ())
		return "\n\n";
	else
		return t.str ();
}

void TokenizedText::listSymbols (std::ostream &output, bool hskTokenIdxs) {
	output << "<SYMBOLS>\n";

	std::list<Symbol *>::const_iterator i = symbList.begin ();
	while (i != symbList.end ()) {

		output << "   <" << (*i)->getTagName ();	// name of element is the name of symbol

		// write symbol positions (token indices)
		if (hskTokenIdxs) {
			output << " from=" << (*i)->getFirstHskPos () << " to=" << (*i)->getLastHskPos ();
		} else {	// FiltNER indices
			output << " first=" << (*i)->getFirstTokenIdx () << " last=" << (*i)->getLastTokenIdx ();
		}

		// write symbol arguments
		(*i)->writeArgs (output);
		output << ">";

		int k;
		for (k = (*i)->getFirstTokenIdx (); k < (*i)->getLastTokenIdx (); ++k) {	// tokens themselves
			output << tokens[k].str () << " ";
		}
		output << tokens[k].str ();	// last token

		output << "</" << (*i)->getTagName () << ">\n"; // closing tag

		i++;
	}

	output << "</SYMBOLS>\n";
}

void TokenizedText::listTaggedText (std::ostream &output) {
	// set ID numbers for Symbols:
	int symbCounter = 1;
	for (std::list<Symbol *>::iterator s = symbList.begin (); s != symbList.end (); ++s) {
		(*s)->setId (symbCounter++);
	}
	
	if (tokens.size () == 0)	// empty text
		return;

	// starting opening tags:
	int i = 0;
	for (std::set<Symbol *>::const_iterator y = tokens[i].getParentSymbols ().begin (); y != tokens[i].getParentSymbols ().end (); ++y) {
		output << "<" << (*y)->getId () << ":" << (*y)->getTagName (); 
		(*y)->writeArgs (output);
		output << ">";
	}

	while (i < tokens.size () - 1) {
		// token itself:
		output << tokenStr(tokens[i]);

		// closing tags:
		for (std::set<Symbol *>::const_iterator x = tokens[i].getParentSymbols ().begin (); x != tokens[i].getParentSymbols ().end (); ++x) {
			if (tokens[i + 1].getParentSymbols ().find (*x) == tokens[i + 1].getParentSymbols ().end ()) {	// ends here if not found
				output << "</" << (*x)->getId () << ":" << (*x)->getTagName () << ">"; 
			}
		}

		// separator:
		output << " ";

		// opening tags:
		for (std::set<Symbol *>::const_iterator y = tokens[i + 1].getParentSymbols ().begin (); y != tokens[i + 1].getParentSymbols ().end (); ++y) {
			if (tokens[i].getParentSymbols ().find (*y) == tokens[i].getParentSymbols ().end ()) {	// starts here if not found
				output << "<" << (*y)->getId () << ":" << (*y)->getTagName ();
				(*y)->writeArgs (output);
				output << ">";
			}
		}
		
		++i;
	}
	// last token itself:
	output << tokenStr(tokens[i]);

	// final closing tags:
	for (std::set<Symbol *>::const_iterator x = tokens[i].getParentSymbols ().begin (); x != tokens[i].getParentSymbols ().end (); ++x) {
		output << "</" << (*x)->getId () << ":" << (*x)->getTagName () << ">"; 
	}
}

int TokenizedText::hskPos2TokenIdx (int hskPos) {
	if (hskPos < 1)
		return -1;
	// estimated starting point as hskPos:
	int tokenIdx = hskPos;

	// if too high:
	if (tokenIdx >= tokens.size ())
		tokenIdx = tokens.size () - 1;

	// seeking backward:
	while (tokenIdx > 0 && (tokens[tokenIdx].getStartPos () == 0 || tokens[tokenIdx].getStartPos () > hskPos))
		--tokenIdx;

	// seeking forward:
	while (tokenIdx < tokens.size () && (tokens[tokenIdx].getEndPos () == 0 || tokens[tokenIdx].getEndPos () < hskPos))
		++tokenIdx;

	if (tokenIdx < 0 || tokenIdx >= tokens.size ()
		|| tokens[tokenIdx].getStartPos () == 0 || tokens[tokenIdx].getStartPos () > hskPos
		|| tokens[tokenIdx].getEndPos () == 0 || tokens[tokenIdx].getEndPos () < hskPos) {
		return -1;	// not found
	} else {
		return tokenIdx;
	}
}
