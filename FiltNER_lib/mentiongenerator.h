#ifndef MENTIONGENERATOR_H
#define MENTIONGENERATOR_H

#include <map>
#include <list>
#include <string>

class TokenizedText;
class Symbol;
class Token;

class MentionGenerator {
private:
	typedef	std::map<std::string, std::list<Symbol *> > SymbolPrefixIndex;
	TokenizedText &tt;
	SymbolPrefixIndex fwIndex;	// first word index
	SymbolPrefixIndex sIndex;	// similarity index

	// keys for grouping symbols (indexing):
	std::string getSimilarityPrefixKey (const Symbol &);		// all tokens but only first two characters of the last one
	std::string getFirstWordAlonePrefixKey (const Symbol &);	// first two characters of first token
	std::string getTokenPrefixKey (int tokenIdx);				// first two characters

	bool isSimilar (int tokenIdx1, int tokenIdx2);	// check similarity of the two tokens (having same stems or not)

	// called by isSimilar:
	bool equalPrefixLongEnough (int prefixLen, int str1Len, int str2Len);	// returns true if the strings of length str1Len and str2Len having
																			// the same prefix of length prefixLen are supposed to have the same stem
																			// (only called for words without known morphology)
	bool isValidNonequalPostfix (const std::string &str, std::string::const_iterator &pos);	// returns true if the suffix of str starting at pos can be
																							// accepted as a standard word suffix (only lower case letters)
																							// [so it is possible for the prefix (without this suffix) to be the stem of the word]
public:
			MentionGenerator (TokenizedText &);
	void	processSimilarity ();		// process 'similarity' heuristics
	void	processFirstWordAlone ();	// process 'first word alone' heuristics
};

#endif /* MENTIONGENERATOR_H */
