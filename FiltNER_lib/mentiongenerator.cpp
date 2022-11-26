#pragma warning(disable:4786 4503)

#include "mentiongenerator.h"
#include "tokenizedtext.h"
#include "symbol.h"

#pragma warning(disable:4786 4503)

MentionGenerator::MentionGenerator (TokenizedText &iTt) : tt (iTt) {}

void MentionGenerator::processSimilarity () {
	// build index:
	std::string key;
	SymbolPrefixIndex::iterator j;
	const std::list<Symbol *> &symbs = tt.getSymbols ();
	std::list<Symbol *>::const_iterator i;
	sIndex.clear ();
	for (i = symbs.begin (); i != symbs.end (); ++i) {
		key = getSimilarityPrefixKey(**i);
		j = sIndex.find (key);
		if (j == sIndex.end ()) { // new entry
			std::list<Symbol *> symbPtrList;
			symbPtrList.push_back (*i);
			sIndex.insert (SymbolPrefixIndex::value_type (key, symbPtrList));
		} else {	// existing entry
			j->second.push_back (*i);
		}
	}

	// check for similar symbols in each group
	for (SymbolPrefixIndex::const_iterator group = sIndex.begin (); group != sIndex.end (); ++group) {
		for (std::list<Symbol *>::const_iterator x = group->second.begin (); x != group->second.end (); ++x) {
			std::list<Symbol *>::const_iterator y = x;
			++y;
			while (y != group->second.end ()) {
				if ((*x)->getName () == (*y)->getName () // same type of symbol
					&& isSimilar ((*x)->getLastTokenIdx (), (*y)->getLastTokenIdx ())) {	// all but last tokens are equal because they have the same key
					(*x)->incCertLev ();
					(*y)->incCertLev ();
				}
				++y;
			}
		}
	}
}

void MentionGenerator::processFirstWordAlone () {
	// build index on multi-token symbols:
	std::string key;
	SymbolPrefixIndex::iterator j;
	const std::list<Symbol *> &symbs = tt.getSymbols ();
	std::list<Symbol *>::const_iterator i;
	fwIndex.clear ();
	for (i = symbs.begin (); i != symbs.end (); ++i) {
		if ((*i)->getNrOfTokens () > 1) {	// only take multi-token symbols
			key = getFirstWordAlonePrefixKey(**i);
			j = fwIndex.find (key);
			if (j == fwIndex.end ()) { // new entry
				std::list<Symbol *> symbPtrList;
				symbPtrList.push_back (*i);
				fwIndex.insert (SymbolPrefixIndex::value_type (key, symbPtrList));
			} else {	// existing entry
				j->second.push_back (*i);
			}
		}
	}

	// find similarities for each one-token symbol with unknown morphology
	for (i = symbs.begin (); i != symbs.end (); ++i) {
		if ((*i)->getNrOfTokens () == 1 && !tt.getTokens ()[(*i)->getFirstTokenIdx ()].hasKnownMorph ()) {
			j = fwIndex.find (getFirstWordAlonePrefixKey(**i));
			if (j != fwIndex.end ()) {	// found multi-token symbols starting with the same first two characters
				// look for symbols with similar first tokens to **i
				for (std::list<Symbol *>::const_iterator k = j->second.begin (); k != j->second.end (); ++k) {
					if ((*k)->getName () == (*i)->getName () // same type of symbol
						&& isSimilar ((*i)->getFirstTokenIdx (), (*k)->getFirstTokenIdx ())) {
						(*i)->incCertLev ();
					}
				}
			}
		}
	}
}

std::string MentionGenerator::getSimilarityPrefixKey (const Symbol &symb) {
	if (symb.getNrOfTokens () <= 0)
		return "";

	std::string s;
	// all but last token:
	for (int i = symb.getFirstTokenIdx (); i < symb.getLastTokenIdx (); ++i) {
		s.append (tt.getTokens ()[i].str ());
		s.append (" ");
	}
	// last token:
	s.append (getTokenPrefixKey (symb.getLastTokenIdx ()));
	return s;
}

std::string MentionGenerator::getFirstWordAlonePrefixKey (const Symbol &symb) {
	return getTokenPrefixKey (symb.getFirstTokenIdx ());
}

std::string MentionGenerator::getTokenPrefixKey (int tokenIdx) {
	std::string s = tt.getTokens ()[tokenIdx].str ();
	
	// preserve the first (at most) two characters:
	if (s.size () > 2)
		s.erase (2);

	return s;
}

bool MentionGenerator::isSimilar (int tokenIdx1, int tokenIdx2) {
	Token &t1 = tt.getTokens ()[tokenIdx1];
	Token &t2 = tt.getTokens ()[tokenIdx2];
	const StringList emptyMorphArgs;
	if (t1.hasKnownMorph () && t2.hasKnownMorph ()) {
		StringList stems1 = t1.getMatchingMorphStems (emptyMorphArgs);
		StringList stems2 = t2.getMatchingMorphStems (emptyMorphArgs);
		for (StringList::const_iterator i = stems1.begin (); i != stems1.end (); ++i) {
			for (StringList::const_iterator j = stems2.begin (); j != stems2.end (); ++j) {
				if (*i == *j) {	// same stem found (string comparison)
					return true;
				}
			}
		}
		return false;	// not found
	} else {
		// check for equal prefix:
		
		std::string::const_iterator x = t1.str ().begin ();
		std::string::const_iterator y = t2.str ().begin ();
		// skip starting punctuation
		if (x != t1.str ().end () && Token::isFirstPunct (*x))
			x++;
		if (y != t2.str ().end () && Token::isFirstPunct (*y))
			y++;

		int prefixLen = 0;
		while (x != t1.str ().end () && y != t2.str ().end () && *x == *y) {
			x++;
			y++;
			prefixLen++;
		}

		// return supposition:
		return (equalPrefixLongEnough (prefixLen, t1.str ().size (), t2.str ().size ()) &&
			isValidNonequalPostfix (t1.str (), x) && isValidNonequalPostfix (t2.str (), y));
	}
}

bool MentionGenerator::equalPrefixLongEnough (int prefixLen, int str1Len, int str2Len) {
	// length heuristics:
	return (4 * prefixLen >= 3 * str1Len && 4 * prefixLen >= 3 * str2Len);
}

bool MentionGenerator::isValidNonequalPostfix (const std::string &str, std::string::const_iterator &pos) {
	if (pos == str.end ())	// empty string is accepted
		return true;

	if (*pos == '-') {	// the first character is allowed to be a '-'
		pos++;
		if (pos == str.end ())	// must be continued
			return false;
	}

	do {
		if (!Token::isLowerCase (*pos))	// found a non-lower-case character
			return false;
		++pos;
	} while (pos == str.end ());

	return true;	// all letters are lower case
}
