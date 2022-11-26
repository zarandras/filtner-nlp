#ifndef SYMBOL_H
#define SYMBOL_H

#include <map>
#include <string>

typedef std::map<std::string, std::string>	SymbArgs;

#define SYMB_SURE		4
#define SYMB_UNSURE		2
#define SYMB_KILLED		0

class Symbol {
private:
	int			 firstTokenIdx, lastTokenIdx;	// FiltNER token index (in TokenizedText)
	int			 firstHskPos, lastHskPos;		// HumorESK position (token) index
	std::string  name;
	SymbArgs	 args;
	short		 certLev;	// uncertainity level
	int			 id;	// used by TokenizedText::listTaggedText ()
public:
	Symbol (const std::string &iName, int iFirstTokenIdx, int iLastTokenIdx, int iFirstHskPos, int iLastHskPos, const SymbArgs &iArgs, short iCertLev = SYMB_SURE, int iId = 0) :
	  id (iId), name (iName), firstTokenIdx (iFirstTokenIdx), lastTokenIdx (iLastTokenIdx), firstHskPos (iFirstHskPos), lastHskPos (iLastHskPos),args (iArgs), certLev (iCertLev)
	  {}
	// copy constructor generated automatically

	inline	int			 getFirstTokenIdx () const	{ return firstTokenIdx; }
	inline	int			 getLastTokenIdx ()	const	{ return lastTokenIdx; }
	inline	int			 getNrOfTokens () const	{ return lastTokenIdx - firstTokenIdx + 1; }

	inline	int			 getFirstHskPos () const	{ return firstHskPos; }
	inline	int			 getLastHskPos ()	const	{ return lastHskPos; }
	
	inline	const std::string	&getName ()	const	{ return name; }
	inline	const std::string	getTagName ()	const	{ if (sure ()) return name; else return name + "?"; }
	const	std::string	*getArgVal (const std::string &argName) const; // output must be copied! returns NULL if no such argument exists
			bool		 deleteArg (const std::string &argName); // returns false if no such argument exists
			void		 transfUnsureArg (const std::string &unsureArgName); // transforms unsure argument to sureLev property

	inline	void		 setCertLev (short newCertLev)	{ certLev = newCertLev; }
	inline	short		 getCertLev () const			{ return certLev; }
	inline	void		 kill ()						{ certLev = SYMB_KILLED; }
	inline	bool		 killed () const				{ return certLev <= SYMB_KILLED; }
	inline	bool		 sure () const					{ return certLev >= SYMB_SURE; }
	inline	void		 incCertLev (short by = 1)		{ certLev += by; }
	inline	void		 decCertLev (short by = 1)		{ certLev -= by; }
	
	// called by TokenizedText:
			void		 writeArgs (std::ostream &) const;	// outputs args starting with a space
	
	// called by TokenizedText::listTaggedText ():
	inline	int			 getId () const					{ return id; }
	inline	void		 setId (int iId)				{ id = iId; }
};

struct greaterSymbLev : public std::greater<Symbol *> {
	bool operator() (const Symbol *&s1, const Symbol *&s2) const { return (s1->getCertLev () > s2->getCertLev ()); }
};

struct greaterSymbPos : public std::greater<Symbol *> {
	bool operator() (const Symbol *&s1, const Symbol *&s2) const { return (s1->getFirstTokenIdx () > s2->getFirstTokenIdx ()
						|| (s1->getFirstTokenIdx () == s2->getFirstTokenIdx () && s1->getLastTokenIdx () > s2->getLastTokenIdx ())); }
};

#endif /* SYMBOL_H */
