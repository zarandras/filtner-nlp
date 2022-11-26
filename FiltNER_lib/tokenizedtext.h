#ifndef TOKENIZEDTEXT_H
#define TOKENIZEDTEXT_H

#pragma warning(disable:4786 4503)

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <list>

#pragma warning(disable:4786 4503)

typedef std::list<std::string>		StringList;

class Symbol;
class Morf;	// Humor wrapper class from morph.h

class Token {
private:
	std::string					 data;
	std::set<Symbol *>			 parentSymbols;
	std::vector<std::string>	*morphology;		// cached morphology - created on first demand by fillMorphology

	int							 startPos;	// starting position in terms of HumorESK tokens
	int							 endPos;	// ending position in terms of HumorESK tokens

	void	fillMorphology ();	//	called by hasKnownMorph and getMatchingMorphStems (morphological analysis on-demand)
	void	resetMorphology ();	//	called by setStr and ~Token (destruct morphology on-demand)

	static Morf					*humor;
	static const int			 humorLID;
public:
	Token (const std::string &iStr, int iStartPos, int iEndPos) :
			data (iStr), morphology (NULL), startPos (iStartPos), endPos (iEndPos) {}
	~Token () { resetMorphology (); }

	inline const std::string &str() const { return data; }
	inline void				  setStr (const std::string &newStr) { data = newStr; resetMorphology (); }
	
	void addParentSymbol (Symbol *s) { parentSymbols.insert (s); }
	bool delParentSymbol (Symbol *s) { return (parentSymbols.erase (s) > 0); }
	const std::set<Symbol *> &getParentSymbols () const { return parentSymbols; }

	// file name constants for Humor
	static const std::string	 humorEngineFileName;
	static const std::string	 humorStemmerFileName;
	static const std::string	 humorLexiconFileName;
	static const std::string	 humorLemmaFileName;

	// should be called to establish connection to Humor
	static void initMorphology (const std::string &basePath, int codePage);	// if basePath is empty, current directory is used
	static void doneMorphology ();
	
	bool hasKnownMorph ();
	StringList getMatchingMorphStems (const StringList &morphArgs);

	// query and set first and last positon (HumorESK token index)
	inline	int	getStartPos () const { return startPos; }
	inline	int	getEndPos () const { return endPos; }
	inline	void setStartEndPos (int startP, int endP) { startPos = startP; endPos = endP; }
	
	// used by CPBENonvar::matchFromBack/Front () and MentionGenerator::isSimilar ()
	// to determine if first or last character can be dropped
	static inline bool	isLastPunct  (char x) { return (x == ')' || x == ']' || x == '}' || x == '.' || x == ';' ||  x == ':' || x == '\'' || x == '"' || x == ',' || x == '!' || x == '?'); } 
	static inline bool	isFirstPunct (char x) { return (x == '(' || x == '[' || x == '{' || x == '\'' || x == '"'); }
	
	// used by MentionGenerator::isValidNonequalPostfix ()
	// to determine if a character is a lower case letter or not
	static inline bool	isLowerCase (char x) { return ((x >= 'a' && x <= 'z') || x == 'á' || x == 'é' || x == 'í' || x == 'ó' || x == 'ö' || x == 'õ' || x == 'ú' || x == 'ü' || x == 'û'); }
};

class TokenizedText {
private:
	std::vector<Token>	tokens;
	std::list<Symbol *> symbList;

	void registerSymbol (Symbol *);		// called by addSymbol
	void unregisterSymbol (Symbol *);	// called by purgeKilledSymbols

	std::set<Symbol *>	sureParentsAt (int idx);
		// returns the certain parent symbols of token at idx
	bool isSureParentAt (int idx) { return (!sureParentsAt (idx).empty ()); }
		// returns true iff token at idx has a certain parent symbol
	bool containedBySure (int firstIdx, int lastIdx);
		// returns true iff tokens [firstIdx..lastIdx] are strictly contained by the same certain parent symbol

	std::string tokenStr (const Token &);	// token data string; empty token is converted to 'new paragraph'
public:
	TokenizedText () {}
	~TokenizedText ();
	
	void addToken (const Token &t) { tokens.push_back (t); }  // copies and adds a token to the end of text
	void addSymbol (const Symbol &);	// copies symbol, registers for tokens - will be deleted by delSymbol or destructor
										// important: call only addSymbol for symbols with already inserted tokens!
	
	inline std::vector<Token> &getTokens () { return tokens; }
	inline const std::list<Symbol *> &getSymbols () const { return symbList; }

	int hskPos2TokenIdx (int);		// converts HumorESK token position into FiltNER token index
									// returns <0 on error
	
	void killOverlappingSymbols (); // property for uncertainity is taken into account
	void purgeKilledSymbols ();	// frees allocated space as well

	void keepMostCertSymbols ();	// eliminate most of the overlapping symbols by keeping the ones with highest certainity level

	// output:
	void listSymbols (std::ostream &, bool hskTokenIdxs);		// list symbols in XML-like format
																//   second parameter determines indexing type (HumorESK/FiltNER tokens)
	void listTaggedText (std::ostream &);	// list whole text and tag symbols
};

#endif /* TOKENIZEDTEXT_H */
