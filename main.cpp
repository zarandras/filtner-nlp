#pragma warning(disable:4786 4503)

#include <iostream>
#include <strstream>
#include <windows.h>

// MorphoLogic includes
extern "C" {
	#include "hsk151_API.h"
}
#include "pathchanger.h"

// FiltNER includes
#include "arghandler.h"
#include "tokenizedtext.h"
#include "symbol.h"
#include "cpcollection.h"
#include "contextpattern.h"
#include "mentiongenerator.h"
#include "paragraphreader.h"

#pragma warning(disable:4786 4503)

#define MAX_PARAGRAPH_LEN	65536

// conversion for SymbArgs from HumorESK API:
void hsk2fnSymbArgs (hsk_pvpair *hsArgs, int hsArgsSize, SymbArgs &fnArgs) {
	if (hsArgs == NULL && hsArgsSize > 0)
		throw FiltNERError ("Invalid symbol arguments.");

	fnArgs.clear ();
	for (int i = 0; i < hsArgsSize; ++i) {
		fnArgs.insert (SymbArgs::value_type (hsArgs[i].szPropertyName, hsArgs[i].szPropertyValue));
	}
}

// process one paragraph
void processParagraph (int parNr, const std::string &par, TokenizedText &tt, const std::string &unsureSymbArgName, int &nextHskPos) {
	if (par.size () >= MAX_PARAGRAPH_LEN) {
		std::ostrstream ss;
		ss << "Paragraph Nr. " << parNr << " is too long.\0";
		throw FiltNERError (ss.str ());
	}

	char parBuf[MAX_PARAGRAPH_LEN];
	strcpy (parBuf, par.c_str ());

	int baseHskPos = nextHskPos - 1;	// each paragraph parsed separately but positions are global

	hsk_symbol_id *hsiRoots;
	hsk_symbol *hsSymbolData;
	int iNumOfRoots, iNumSymbols, iNumPositions;

	HSKAPI_ERROR haeError = haeParse (parBuf, &hsiRoots, &iNumOfRoots, &iNumSymbols, &iNumPositions);

	if (haeError == HSKAPI_ERROR_INCOMPLETE) {
		std::ostrstream ss;
		ss << "HumorESK: incomplete parse at paragraph Nr. " << parNr << " (too many symbols to be generated)\0";
		throw FiltNERError (ss.str ());
	} else if (haeError != HSKAPI_OK) {
		std::ostrstream ss;
		ss << "HumorESK: parse error Nr. " << haeError << " at paragraph Nr. " << parNr << "\0";
		throw FiltNERError (ss.str ());
	}

	// construct Tokens
	bool noTokens = true;	// for indicating error
	for (int i = 0; i < iNumOfRoots; i++) {
		haeGetSymbol (hsiRoots[i], &hsSymbolData);
		if (strcmp (hsSymbolData->szClassName, "ANY") != 0)
			continue;	// non-ANY symbols are no tokens

		noTokens = false;	// found a token

		// validity check
		if (hsSymbolData->iEndingPosition < hsSymbolData->iStartingPosition || hsSymbolData->szSurface[0] == '\0') {
			std::ostrstream ss;
			ss << "HumorESK: illegal 'ANY' symbol parsed at paragraph Nr. " << parNr << " (starting position in paragraph: " << hsSymbolData->iStartingPosition << ", ending position: " << hsSymbolData->iEndingPosition << ", surface string: '" << hsSymbolData->szSurface << "').\0";
			throw FiltNERError (ss.str ());
		} else if (nextHskPos != hsSymbolData->iStartingPosition + baseHskPos) {
			std::ostrstream ss;
			ss << "HumorESK: inconsistent parse or 'ANY' symbols are not properly ordered at paragraph Nr. " << parNr << " - expected token position is " << nextHskPos << ", received " << hsSymbolData->iStartingPosition + baseHskPos << " (" << hsSymbolData->iStartingPosition << " in paragraph) instead.\0";
			throw FiltNERError (ss.str ());
		}

		// construct token and add to tt
		Token t (hsSymbolData->szSurface, hsSymbolData->iStartingPosition + baseHskPos, hsSymbolData->iEndingPosition + baseHskPos);
		tt.addToken (t);
		nextHskPos = hsSymbolData->iEndingPosition + baseHskPos + 1;
	}

	// no 'ANY'-type symbols found. Possible reason: ANY is not indicated in TAGS.TABLE
	if (noTokens) {
		std::ostrstream ss;
		ss << "No tokens are parsed by HumorESK at paragraph Nr. " << parNr << ". [Make sure 'ANY' is included in root symbols table (TAGS.TABLE).]\0";
		throw FiltNERError (ss.str ());
	}


	// construct Symbols
	for (i = 0; i < iNumOfRoots; i++) {
		haeGetSymbol (hsiRoots[i], &hsSymbolData);
		if (strcmp (hsSymbolData->szClassName, "ANY") == 0)
			continue;	// ANY symbols are already processed as tokens

		// validity check
		if (hsSymbolData->iEndingPosition < hsSymbolData->iStartingPosition) {
			std::ostrstream ss;
			ss << "HumorESK: illegal 'ANY' symbol parsed at paragraph Nr. " << parNr << " (starting position in paragraph: " << hsSymbolData->iStartingPosition << ", ending position: " << hsSymbolData->iEndingPosition << ", surface string: '" << hsSymbolData->szSurface << "').\0";
			throw FiltNERError (ss.str ());
		} else if (nextHskPos <= hsSymbolData->iEndingPosition + baseHskPos) {
			std::ostrstream ss;
			ss << "HumorESK: inconsistent parse at paragraph Nr. " << parNr << " - expected maximal ending position is " << nextHskPos - 1 << ", received " << hsSymbolData->iEndingPosition + baseHskPos << " (" << hsSymbolData->iEndingPosition << " in paragraph) instead.\0";
			throw FiltNERError (ss.str ());
		}
		int firstTokenIdx = tt.hskPos2TokenIdx (hsSymbolData->iStartingPosition + baseHskPos);
		int lastTokenIdx = tt.hskPos2TokenIdx (hsSymbolData->iEndingPosition + baseHskPos);
		if (firstTokenIdx < 0 || lastTokenIdx < 0) {
			std::ostrstream ss;
			ss << "FiltNER cannot transform HumorESK token positions to token indices at at paragraph Nr. " << parNr << " (starting position in paragraph: " << hsSymbolData->iStartingPosition << ", ending position: " << hsSymbolData->iEndingPosition << ", global base position for paragraph: " << baseHskPos << ").\0";
			throw FiltNERError (ss.str ());
		}
		
		// get symbol arguments
		SymbArgs sArgs;
		hsk2fnSymbArgs (hsSymbolData->hpvpPropertyString, hsSymbolData->iPropertyStringSize, sArgs);

		// construct symbol
		Symbol s (hsSymbolData->szClassName,
			firstTokenIdx, lastTokenIdx,
			hsSymbolData->iStartingPosition + baseHskPos, hsSymbolData->iEndingPosition + baseHskPos,
			sArgs);
			
		// transform uncertainity argument to numeric uncertainity level
		s.transfUnsureArg (unsureSymbArgName);	// nothing happens if unsureSymbArgName is empty

		// add to tt
		tt.addSymbol (s);	
	}
}



int main(int argc, char *argv[]) {
  int retVal;
  try {
	
	std::cout <<
				 "-------------------------------------------------------------\n" <<
				 "FiltNER named entity recognizer v1.0, (c) Molnar Andras 2003\n" <<
			     "  using\n" <<
			     "HumorESK parser, (c) MorphoLogic Kft. 2003\n" <<
			     "  and\n" <<
				 "Humor morphological analyzer, (c) MorphoLogic Kft. 2002\n" <<
				 "-------------------------------------------------------------\n" <<
				 "\n";

	ArgHandler			theArgHandler (argc, argv);						// throws ArgError
	
	TokenizedText theText;
	int grammarCP;	// code page: retrieved from HumorESK, passed to Humor

	// read text:
	{
		// change to HumorESK directory if given
		PathChanger pathChanger (theArgHandler.getHumorESKDir ());

		// start HumorESK parser
		HSKAPI_ERROR haeError = haeStartParser (const_cast <char *> (theArgHandler.getHumorESKConfFileName ().c_str ()),
												const_cast <char *> (theArgHandler.getFilterName ().c_str ()),
												0,
												const_cast <char *> (theArgHandler.getRootSymbTableName ().c_str ()),
												&grammarCP,
												NULL);
		if (haeError != HSKAPI_OK) {
			std::ostrstream ss;
			ss << "HumorESK initialization error Nr. " << haeError << "\0";
			throw FiltNERError (ss.str ());
		}
		std::cout << "HumorESK parser started. Using codepage Nr. " << grammarCP << ".\n";
				
		// insert empty token as first ('new paragraph')
		Token  t ("", 0, 0);
		theText.addToken (t);

		// read paragraphs
		try {
			int nextHskPos = 1;
			int parNr = 1;
			std::string paragraph;
			ParagraphReader reader (theArgHandler.getInputTextStream ());
			while (!reader.noMore ()) {
				reader.readNext (paragraph);

				processParagraph (parNr++, paragraph, theText, theArgHandler.getUnsureArgName (), nextHskPos);	// increments nextHskPos as well
				
				theText.addToken (t);	// insert empty token with zero positions as 'new paragraph'
			}

			parNr--;
			std::cout << "Successfully parsed " << parNr << " paragraph" << ((parNr == 1) ? "" : "s") <<".\n";
		
			// close input stream
			theArgHandler.closeInputTextStream ();

			// close HumorESK parser
			haeError = haeStopParser ();
			if (haeError != HSKAPI_OK) {
				std::cerr << "Warning: HumorESK deinitialization error Nr. " << haeError << ".\n";
			}
		} catch (...) {
			theArgHandler.closeInputTextStream ();
			haeStopParser ();
			throw;
		}

		// automatically reverting to original directory by calling destructor of pathChanger
	}

	// global Humor initialization for on-demand morphology of tokens
	Token::initMorphology (theArgHandler.getMorphDir (), grammarCP);

	// filter overlapping symbols (1)
	theText.killOverlappingSymbols ();
	theText.purgeKilledSymbols ();

	// process ContextPatterns if given
	if (!theArgHandler.getContextPatternFileName ().empty ()) {
		// change to ContextPattern directory if given
		PathChanger pathChanger (theArgHandler.getContextPatternDir ());

		CPCollection		theCPs (theArgHandler.getContextPatternFileName ());	// throws CPError
		theCPs.process (theText);
		theText.purgeKilledSymbols ();

	// filter overlapping symbols (2)
		theText.killOverlappingSymbols ();
		theText.purgeKilledSymbols ();
		// automatically reverting to original directory by calling destructor of pathChanger
	}

	// process MentionGenerator
	if (!theArgHandler.isMentionGeneratorDisabled ()) {
		MentionGenerator mg (theText);
		mg.processSimilarity ();
		mg.processFirstWordAlone ();
	}

	// generate result
	theText.keepMostCertSymbols ();
	if (theArgHandler.isTaggedTextOption ()) {
		theText.listTaggedText (theArgHandler.getOutputTextStream ());
	} else {
		theText.listSymbols (theArgHandler.getOutputTextStream (), theArgHandler.isHumorESKTokenPosOption ());
	}

	retVal = 0;

  } catch (AHHelp &e) {	// not real error - user asked for command line syntax
		std::cout << "\n";
		e.printArgSyntax (std::cout);
		std::cout << "\n";
		retVal = 0;
  } catch (AHError &e) {
		std::cerr << "FiltNER: Argument error occured:\n   " << e.what () << "\n";
		retVal = 1;
  } catch (CPError &e) {
		std::cerr << "FiltNER: ContextPattern error occured:\n   " << e.what () << "\nProgram terminated.\n";
		retVal = 2;
  } catch (FiltNERError &e) {
		std::cerr << "FiltNER: General error occured:\n   " << e.what () << "\nProgram terminated.\n";
		retVal = 2;
/* !!! visszatenni !!! 
  } catch (exception &e) {
		std::cerr << "FiltNER: Critical system or program error occured:\n   " << e.what () << "\nProgram terminated.\n";
		retVal = 3;
  } catch (...) {
		std::cerr << "FiltNER: Unknown critical system or program error occured. Program terminated.\n";
		retVal = 3;
*/  }

  Token::doneMorphology ();
  return retVal;
}
