#ifndef ARGHANDLER_H
#define ARGHANDLER_H

#pragma warning(disable:4786 4503)

#include <iostream>
#include <fstream>
#include <string>

#pragma warning(disable:4786 4503)

#include "filtnererror.h"

class AHError : public FiltNERError {
public:
	AHError (std::string iStr) : FiltNERError (iStr + " (use /? to get help)") {}
};

class AHHelp {
private:
	static	const std::string	 mainSyntax, mlSyntax, fnSyntax, ioSyntax;
public:
	void printArgSyntax (std::ostream &os) { os << mainSyntax << mlSyntax << fnSyntax << ioSyntax; }
};

class ArgHandler {
private:
				  std::ifstream	 inputTextStream;
				  std::ofstream	 outputTextStream;
				  std::string	 contextPatternFileName;
				  std::string	 contextPatternDir;
				  std::string	 unsureArgName;
				  std::string	 rootSymbTableName;
				  std::string	 filterName;
				  std::string	 humorESKConfFileName;
				  std::string	 humorESKDir;
				  std::string	 morphDir;
				  bool			 mentionGeneratorDisabled;
				  bool			 taggedTextOption;
				  bool			 humorESKTokenPosOption;

	inline	bool	isSwitch (int i, int argc, char **argv) { return (argv[i][0] == '/' && argv[i][1] != '\0' && argv[i][2] == '\0'); }
	inline	bool	isNonSwitch (int i, int argc, char **argv) { return (i < argc && argv[i][0] != '/'); }
			void	storeArg (std::string &dest, const std::string &argName, int i, int argc, char **argv); // if dest is not empty, throws an error
			void	checkCompulsoryArgument (const std::string &argValue, const std::string &syntaxInfo, const std::string &infoText); // throws an error if argValue is empty
			bool	checkOptionalArgument (const std::string &argValue, const std::string &infoText);	// returns false if argValue is empty
			void	writeProcessing (const std::string &val, const std::string &as); // cout << "Using <val> as <as>..."
public:
									 ArgHandler (int argc, char **argv);
									~ArgHandler ();
					void			 closeInputTextStream ();
					void			 closeOutputTextStream ();

					std::istream	&getInputTextStream ();
					std::ostream	&getOutputTextStream ();
	inline	const	std::string		&getContextPatternFileName () const		{ return contextPatternFileName; }
	inline	const	std::string		&getContextPatternDir () const			{ return contextPatternDir; }
	inline	const	std::string		&getUnsureArgName () const				{ return unsureArgName; }
	inline	const	std::string		&getRootSymbTableName () const			{ return rootSymbTableName; }
	inline	const	std::string		&getFilterName () const					{ return filterName; }
	inline	const	std::string		&getHumorESKConfFileName () const		{ return humorESKConfFileName; }
	inline	const	std::string		&getHumorESKDir () const				{ return humorESKDir; }
	inline	const	std::string		&getMorphDir () const					{ return morphDir; }
	inline			bool			 isMentionGeneratorDisabled () const	{ return mentionGeneratorDisabled; }
	inline			bool			 isTaggedTextOption () const			{ return taggedTextOption; }
	inline			bool			 isHumorESKTokenPosOption () const		{ return humorESKTokenPosOption; }
};

#endif /* ARGHANDLER_H */
