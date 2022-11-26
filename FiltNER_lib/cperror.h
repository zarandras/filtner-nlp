
// Do not include it directly. Include through contextpattern.h !

#ifndef CPERROR_H
#define CPERROR_H

#include <string>
#include "filtnererror.h"

std::string char2str (char c);

class CPError : public FiltNERError {
public:
	CPError (const std::string &iStr) : FiltNERError (iStr) {}
};


class CPParseError : public CPError {
public:
	CPParseError (const std::string &iStr, const std::string &ruleStr) : CPError ("PARSE ERROR: " + iStr + "\n  Rule line: < " + ruleStr + " >") {}
};

class CPParseErrorEoln : public CPParseError {
public:
	CPParseErrorEoln (const std::string &ruleStr) : CPParseError ("Unexpected end of line.", ruleStr) {}
};

class CPParseErrorChar : public CPParseError {	// given character expected
public:
	CPParseErrorChar (const std::string &ruleStr, char found, char expected) :	// found \0: end of line
		CPParseError ("Character " + char2str (expected) + " expected, " + ((found == '\0') ? "end of line" : char2str (found)) + " found.", ruleStr)
		{}
};

class CPParseErrorChars : public CPParseError {	// one of the given chars expected
public:
	CPParseErrorChars (const std::string &ruleStr, char found, std::string expectedChars) :	// found \0: end of line
		CPParseError ("A character of < " + expectedChars + " > expected, " + ((found == '\0') ? "end of line" : char2str (found)) + " found instead.", ruleStr)
		{}
};

class CPParseErrorTxt : public CPParseError {	// invalid character in quoted text
public:
	CPParseErrorTxt (const std::string &ruleStr, char found) :	// found \0: end of line
		CPParseError ("Invalid character \\" + ((found == '\0') ? "n (end of line)" : char2str (found)) + " in quoted text.", ruleStr)
		{}
};

class CPParseErrorIllegalUse : public CPParseError {	// illegal use of special character in quoted text
public:
	CPParseErrorIllegalUse (const std::string &ruleStr, char found) :	// found \0: end of line
		CPParseError ("Illegal use of character \\" + ((found == '\0') ? "n (end of line)" : char2str (found)) + " in quoted text.", ruleStr)
		{}
};

class CPParseErrorName : public CPParseError {	// name or number expected
public:
	CPParseErrorName (const std::string &ruleStr, char found) :	// found \0: end of line
		CPParseError ("An alphanumeric name identifier expected, " + ((found == '\0') ? "end of line" : char2str (found)) + " found instead.", ruleStr)
		{}
};

class CPValidityError : public CPError {
public:
	CPValidityError (const std::string &iStr, const std::string &ruleStr, const std::string &cp) : CPError ("INVALID RULE: " + iStr + "\n   Rule line: < " + ruleStr + " >\n   Parsed: < " + cp + " >") {}
};

class CPValidityErrorTooManyBEs : public CPValidityError {	// too many body elements
public:
	CPValidityErrorTooManyBEs (const std::string &ruleStr, const std::string &cp) : CPValidityError ("Too many elements in rule body.", ruleStr, cp) {}
};

class CPValidityErrorTooManyVars : public CPValidityError {	// too many variables
public:
	CPValidityErrorTooManyVars (const std::string &ruleStr, const std::string &cp) : CPValidityError ("Too many variables in rule body.", ruleStr, cp) {}
};

class CPValidityErrorUndeclaredVar : public CPValidityError {			// undeclared variable used in head
public:
	CPValidityErrorUndeclaredVar (const std::string &ruleStr, const std::string &cp, std::string v) :
		CPValidityError ("Undeclared symbol variable " + v + " found in rule head.", ruleStr, cp) 
		{}
};

class CPValidityErrorHeadMultiVar : public CPValidityError {			// variable used in head more than once
public:
	CPValidityErrorHeadMultiVar (const std::string &ruleStr, const std::string &cp, std::string v) :
		CPValidityError ("Duplicated variable " + v + " found in rule head.", ruleStr, cp) 
		{}
};

class CPValidityErrorMultideclaredVar : public CPValidityError {			// multideclared variable in body
public:
	CPValidityErrorMultideclaredVar (const std::string &ruleStr, const std::string &cp, std::string v) :
		CPValidityError ("Symbol variable " + v + " is multideclared in rule body.", ruleStr, cp) 
		{}
};

class CPValidityErrorUndeclaredList : public CPValidityError {			// undeclared list
public:
	CPValidityErrorUndeclaredList (const std::string &ruleStr, const std::string &cp, std::string s) :
		CPValidityError ("Undeclared list variable #" + s + " found in rule body.", ruleStr, cp) 
		{}
};

#endif /* CPERROR_H */