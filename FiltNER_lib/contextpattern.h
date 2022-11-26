#ifndef CONTEXTPATTERN_H
#define CONTEXTPATTERN_H

#include <string>
#include <map>
#include <list>
#include <set>
#include <vector>
#include "filtnererror.h"
#include "symbol.h"
#include "cperror.h"
#include "txtlistmap.h"
#include "tokenizedtext.h"

#define MAX_BELEMS	16
#define MAX_VARS	 3

typedef enum {Pat_None, Pat_Prefix, Pat_Suffix} TPatType;	// used by CPBESTxt and CPBEVar

// CPBElem

class ContextPattern;

class CPBElem {
public:
	virtual operator std::string () = 0;
	virtual ~CPBElem () {}

	// polymorphic recursive functions for matching in cooperation with ContextPattern::matchAndApplyPre/Suffix ():
	virtual bool	matchAndApplyCPPrefix (ContextPattern &, int beIdx, TokenizedText &, int tokenIdx) = 0;
	virtual bool	matchAndApplyCPSuffix (ContextPattern &, int beIdx, TokenizedText &, int tokenIdx, std::list <Symbol *> &matchingSymbs) = 0;
protected:
					 CPBElem ()							{} // disabled for outside
					 CPBElem (const CPBElem &)			{}	// disabled
	const	CPBElem &operator= (const CPBElem &)		{}	// disabled
			bool	 operator== (const CPBElem &)		{}	// disabled
};

class CPBENonvar : public CPBElem {
private:
	StringList		*morphArgs;

protected:
	virtual std::string strPrefix () = 0;			// called by operator std::string

	// called by matchFromFront/Back:
	virtual int		getNrOfTokenPatterns () = 0;	// nr. of token patterns
	virtual bool	matchesTokenPattern (const std::string &tokenStr, int tokenPatIdx) = 0;
													// token string matches pattern nr. tokenStrIdx or not
			bool	matchMorphology (Token &t, int pos);	// called for last word if morphArgs is not NULL
			bool	matchesCutFirstPunct (std:: string &s, int pos);	// calls matchesTokenPattern without the first char if it's a punctuation mark
			bool	matchesCutLastPunct (std:: string &s, int pos);		// calls matchesTokenPattern without the last char if it's a punctuation mark

public:
	CPBENonvar () : morphArgs (NULL) {}
	~CPBENonvar () { if (morphArgs != NULL) delete morphArgs; }
	void	setMorphArgs (StringList *);	// can be called only once (otherwise throws an error)
											// because destructor frees StringList*
	virtual operator std::string ();

	virtual bool	matchAndApplyCPPrefix (ContextPattern &, int beIdx, TokenizedText &, int tokenIdx);
	virtual bool	matchAndApplyCPSuffix (ContextPattern &, int beIdx, TokenizedText &, int tokenIdx, std::list <Symbol *> &matchingSymbs);

	// mainly called by matchAndApplyCPPre/Suffix - returns < 0 if no match, otherwise returns the index of last/first token of match:
	virtual int	matchFromFront (TokenizedText &, int tokenIdx);
	virtual int	matchFromBack (TokenizedText &, int tokenIdx);
};

class CPBESTxt : public CPBENonvar {
	std::vector<std::string>	surfTokens;
	TPatType					patType;
public:
	CPBESTxt (const std::string iSurf, TPatType iPatType);
	virtual std::string strPrefix ();

	virtual int		getNrOfTokenPatterns () { return surfTokens.size (); }
	virtual bool	matchesTokenPattern (const std::string &tokenStr, int tokenPatIdx);
};

class CPBESLst : public CPBENonvar {
	const StringSet &txtList;
public:
	CPBESLst (const StringSet &iTxtList) : txtList (iTxtList) {}
	virtual std::string strPrefix ();

	virtual int		getNrOfTokenPatterns () { return 1; }	// each #list corresponds to one token
	virtual bool	matchesTokenPattern (const std::string &tokenStr, int tokenPatIdx);
};

class CPBEVar : public CPBElem {		// X{...}:(...) | X:(...) | X
public:
	typedef enum {Op_None, Op_Kill, Op_Unsure, Op_Sure} Op;

private:
	std::string							varName;
	std::string							symbName;
	SymbArgs						   *symbArgs;
	TPatType							patType;
	CPBENonvar						   *patStrElem;
	Op									op;			// operator: . | ! | ?
public:
	CPBEVar (std::string iVarName, std::string iSymbName, SymbArgs *iSymbArgs = NULL, TPatType iPatType = Pat_None, CPBENonvar *iPatStrElem = NULL, Op iOp = Op_None) :
	  varName (iVarName), symbName (iSymbName), symbArgs (iSymbArgs), patType (iPatType), patStrElem (iPatStrElem), op (iOp)
	  {}
	~CPBEVar ()			{ if (symbArgs != NULL) delete symbArgs;
						  if (patStrElem != NULL) delete patStrElem; }
	virtual operator std::string ();

	inline	const std::string &getVarName () { return varName; }
	inline	const std::string &getSymbName () { return symbName; }
	inline	void	setOp (Op newOp)	{ op = newOp; }
	inline	Op		getOp ()			{ return op; }
			void	applyOp (Symbol &);
			bool	matchSymb (Symbol &, TokenizedText &);

	virtual bool	matchAndApplyCPPrefix (ContextPattern &, int beIdx, TokenizedText &, int tokenIdx);
	virtual bool	matchAndApplyCPSuffix (ContextPattern &, int beIdx, TokenizedText &, int tokenIdx, std::list <Symbol *> &matchingSymbs);
};

// LL(1) parsing

class ContextPattern;

class CPParser {
private:
	const	std::string						   &source;
			std::string::const_iterator			i;
			ContextPattern					   &dest;
	const	TxtListMap						   &txtLists;
public:
		 CPParser (const std::string &iSource, ContextPattern &iDest, const TxtListMap &iTxtLists);

	void parse () throw (CPError);
private:
	// iterator functions
	void initIter ();	// seeks to first non-whitespace
	void stepIter ();	// steps and seeks to next non-whitespace
	void seekIter ();	// seeks to next non-whitespace
	char atIter ();		// *i if not at end, \0 on end
	
	// for terminal symbols
	void acceptChar (char);							// accepts expected char and seeks to next non-whitespace
	void acceptChar () { stepIter (); }				// accepts next char and seeks to next non-whitespace
	void acceptTxt (std::string &, TPatType *);		// string between "-s, if TPatType* is given, \* is allowed as first or last character
	void acceptName (std::string &);				// a word consists of letters, numbers or _-s

	// for nonterminals
	void				 parseBody ();
	void				 parseHead ();
	void				 parseHElement ();
	void				 parseHRest ();
	void				 parseHEOperator (CPBEVar::Op &op);
	CPBElem				*parseBElement ();
	void				 parseBRest ();
	CPBENonvar			*parseBENonvar ();
	CPBEVar				*parseBEVar ();
	CPBENonvar			*parseBEStr ();
	StringList			*parseMArgs ();
	void				 parseMArgList (StringList &mArgs);
	void				 parseMArgRest (StringList &mArgs);
	void				 parseBEVarDef (TPatType &patType, CPBENonvar *&patStrElem, std::string &symb, SymbArgs *&symbArgs);
	CPBENonvar			*parseVarPattern (TPatType &patType);
	void				 parseSymb (std::string &symb, SymbArgs *&symbArgs);
	SymbArgs			*parseSArgs ();
	void				 parseSArgList (SymbArgs &sArgs);
	void				 parseSArgRest (SymbArgs &sArgs);
	void				 parseSArg (SymbArgs &sArgs);
	void				 parseSArgVal (std::string &s);
};

// ContextPattern

class ContextPattern {
private:
	CPBElem *body[MAX_BELEMS]; short bodyLen;
	int		 vars[MAX_VARS]; short varsLen;


							ContextPattern (const ContextPattern &)	{}	// disabled
	const	ContextPattern &operator= (const ContextPattern &)		{}	// disabled
			bool			operator== (const ContextPattern &)		{}	// disabled

			void			addBodyElem	(CPBElem *, const std::string &sourceForError); // called by CPParser; immediately deletes pointer on error, otherwise destructor deletes
			CPBEVar		   *getVarByName (const std::string &varName);			// returns NULL if not declared

public:
							 ContextPattern (const std::string &source, const TxtListMap &txtLists);	// parse from source line
							~ContextPattern ();
							 operator std::string ();
			CPBEVar			&ContextPattern::getFirstCPBEVar ();
	const	std::string		&getFirstSymbName ();

			bool			 matchAndApply (Symbol &, TokenizedText &);
								// if this pattern matches at Symbol&, apply operators to symbol(s)

			// recursive functions for matching in cooperation with CPBE...::matchAndApplyCPPre/Suffix ():
			//	(called by matchAndApply and CPBE...::matchAndApplyCPPre/Suffix ())
			bool			matchAndApplyPrefix (int beIdx, TokenizedText &tt, int tokenIdx);
			bool			matchAndApplySuffix (int beIdx, TokenizedText &tt, int tokenIdx, std::list<Symbol *> &matchingSymbs);

	friend class CPParser;	// only calls private functions - does not modifies elements
};

#endif /* CONTEXTPATTERN_H */
