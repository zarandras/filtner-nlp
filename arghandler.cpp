
#pragma warning(disable:4786 4503)

#include <iostream>
#include <fstream>
#include "arghandler.h"

#pragma warning(disable:4786 4503)

const std::string    AHHelp::mainSyntax = "\
Command line syntax:\n\
\n\
  FiltNER.EXE  /G humorESK_conf_file\n\
              [/H humorESK_directory]        [/M humor_directory]\n\
              [/R root_symbol_table]         [/F filter_name]\n\
              [/C context_pattern_file_name] [/D context_pattern_directory]\n\
              [/U unsure_arg_name]           [/N]\n\
              [/I input_text_file_name]\n\
              [/O output_text_file_name]     [/T | /P]\n\
\n";

const std::string    AHHelp::mlSyntax = "\
Humor & HumorESK arguments:\n\
\n\
    /G filename       - HumorESK configuration file (like HU01.GCONFIG)\n\
                        It has to be in the HumorESK directory which is\n\
                        the current directory if no /H specified (and\n\
                        all references in the file must be relative to\n\
                        this directory). This is a compulsory argument.\n\
    /H directory      - HumorESK directory in which the files required for\n\
                        parsing has to be placed. This folder must also\n\
                        contain the TAGS.TABLE file with root symbol table.\n\
                        The current directory is taken as default.\n\
    /M directory      - Humor directory which must contain all the files\n\
                        required for Humor morphological analysis including\n\
                        ML6MOR32.DLL, STEM.DLL, HUAU.LEX and HUSTEM.LEX.\n\
                        It is not neccessary for FiltNER to use the same\n\
                        version of Humor as HumorESK does.\n\
                        The current directory is taken as default.\n\
    /R identifier     - Name of the root symbol table in TAGS.TABLE (without\n\
                        brackets). It must be an existing section name of\n\
                        the file and this table should contain the names\n\
                        of root named entity symbol classes as well as the\n\
                        'ANY' special symbol name.\n\
                        The name 'Default' is taken as the default.\n\
    /F identifier     - Name of the filter used for HumorESK parsing. This\n\
                        must be defined in the configuration file specified\n\
                        at /G. Filter 'plain_hlem' is taken as the default.\n\
\n";

const std::string    AHHelp::fnSyntax = "\
FiltNER arguments:\n\
\n\
    /U identifier     - Name of the uncertainity argument of named entity\n\
                        symbols. It must be the name of a YES/NO argument of\n\
                        symbol classes which was specified by transforming\n\
                        unsure MetaMorpho rules to normal MMO format with\n\
                        the PreRule precompiler. All symbols having YES\n\
                        as the value of this argument will initially be\n\
                        treated as uncertain at the beginning.\n\
                        There is no default value: if /U is not specified\n\
                        this feature is switched off.\n\
                        Suggested value: 'unsure'.\n\
    /C filename       - Name of the context pattern (.CP) file which\n\
                        contains all the context patterns in source format\n\
                        and the required word list filenames. It has to be\n\
                        in the ContextPattern directory which is the current\n\
                        directory if no /D is specified (and all references\n\
                        in the file must be relative to this directory).\n\
                        There is no default value: if /C is not specified\n\
                        ContextPattern filtering is disabled.\n\
    /D directory      - ContextPattern directory in which the context pattern\n\
                        file has to be placed. /D cannot be used without /C.\n\
                        The current directory is taken as default.\n\
    /N                - If specified, MentionGenerator filtering is\n\
                        disabled. It is enabled by default.\n\
\n";

const std::string    AHHelp::ioSyntax = "\
Input & Output arguments:\n\
\n\
    /I filename       - Name of the input text file. It must be a plain\n\
                        text with the same characted code page as the\n\
                        HumorESK grammar (specified by /G) was made for.\n\
                        Default is standard input.\n\
    /O filename       - Name of the output text file. Output format is\n\
                        determined by arguments /T and /P.\n\
                        Default is standard output.\n\
    /T                - If specified, output is similar to the original\n\
                        input text with the found named entities tagged.\n\
                        If not specified, output is a list of found\n\
                        named entity symbols.\n\
    /P                - Determines the mode of indicating the starting and\n\
                        ending position mode for symbol listing output:\n\
                        If specified, starting and ending positions are\n\
                        written in terms of HumorESK token indices.\n\
                        If not specified, positions are written as\n\
                        FiltNER token indices. Note the different\n\
                        syntax for the two kinds of output.\n\
                        This argument cannot be used when /T is specified.\n\
\n";

void ArgHandler::storeArg (std::string &dest, const std::string &argName, int i, int argc, char **argv) {
    if (!dest.empty ()) {
        throw AHError ("Duplicate argument passed: " + argName);
    }
    if (!isNonSwitch (i, argc, argv)) {
        throw AHError ("No parameter provided for " + argName);                    
    }    
    dest = argv[i];
}


void ArgHandler::checkCompulsoryArgument (const std::string &argValue, const std::string &syntaxInfo, const std::string &infoText) {
    if (argValue.empty ()) {
        throw AHError ("No " + infoText + " specified. Use '" + syntaxInfo + "'.");
    } else {
        writeProcessing (argValue, infoText);
    }
}

bool ArgHandler::checkOptionalArgument (const std::string &argValue, const std::string &infoText) {
    if (argValue.empty ()) {
        return false;
    } else {
        writeProcessing (argValue, infoText);
        return true;
    }
}

void ArgHandler::writeProcessing (const std::string &val, const std::string &as) {
    std::cout << "Using " << val << " as " << as << "...\n";
}

ArgHandler::ArgHandler (int argc, char **argv) :
    mentionGeneratorDisabled (false),
    taggedTextOption (false),
    humorESKTokenPosOption (false),
    rootSymbTableName ("Default"),
    filterName ("plain_hlem")
{

    // must have at least one argument
    if (argc == 1) {
        throw AHError ("No arguments passed.");
    }

    std::string inputTextFileName, outputTextFileName;
    int i = 1;
    while (i < argc) {
        // argument must start with / and must contain exactly one more character
        if (!isSwitch (i, argc, argv)) {
            std::string s = "Invalid argument passed (";
            throw AHError (s + argv[i] + ").");
        }
        switch (argv[i][1]) {
            case '?':
                throw AHHelp ();    // to be catched by main function
                break;

            case 'I':
            case 'i':
                ++i;
                storeArg (inputTextFileName, "/I", i, argc, argv);
                break;

            case 'O':
            case 'o':
                ++i;
                storeArg (outputTextFileName, "/O", i, argc, argv);
                break;

            case 'C':
            case 'c':
                ++i;
                storeArg (contextPatternFileName, "/C", i, argc, argv);
                break;

            case 'D':
            case 'd':
                ++i;
                storeArg (contextPatternDir, "/D", i, argc, argv);
                break;

            case 'U':
            case 'u':
                ++i;
                storeArg (unsureArgName, "/U", i, argc, argv);
                break;

            case 'R':
            case 'r':
                ++i;
                storeArg (rootSymbTableName, "/R", i, argc, argv);
                break;

            case 'F':
            case 'f':
                ++i;
                storeArg (filterName, "/F", i, argc, argv);
                break;

            case 'H':
            case 'h':
                ++i;
                storeArg (humorESKDir, "/H", i, argc, argv);
                break;

            case 'G':
            case 'g':
                ++i;
                storeArg (humorESKConfFileName, "/G", i, argc, argv);
                break;

            case 'M':
            case 'm':
                ++i;
                storeArg (morphDir, "/M", i, argc, argv);
                break;

            case 'N':
            case 'n':
                mentionGeneratorDisabled = true;
                break;

            case 'T':
            case 't':
                taggedTextOption = true;
                break;

            case 'P':
            case 'p':
                humorESKTokenPosOption = true;
                break;

            default:
                std::string s = "Invalid argument passed (";
                throw AHError (s + argv[i] + ").");
        }
        ++i;
    }

    // check compulsory arguments

    checkCompulsoryArgument (humorESKConfFileName, "/G file_name", "HumorESK configuration file");

    // promt optional arguments

    if (!checkOptionalArgument (inputTextFileName, "input text")) {
        writeProcessing ("standard input", "input text");
    } else {
        inputTextStream.open (inputTextFileName.c_str ());
        if (!inputTextStream.is_open ()) {
            throw FiltNERError ("Cannot open input text file.");
        }
    }

    if (!checkOptionalArgument (outputTextFileName, "output file")) {
        writeProcessing ("standard output", "output file");
    } else {
        outputTextStream.open (outputTextFileName.c_str ());
        if (!outputTextStream.is_open ()) {
            throw FiltNERError ("Cannot open output file.");
        }
    }

    if (!checkOptionalArgument (morphDir, "Humor directory")) {
        writeProcessing ("current directory", "Humor directory");
    }

    if (!checkOptionalArgument (humorESKDir, "HumorESK directory")) {
        writeProcessing ("current directory", "HumorESK directory");
    }

    if (taggedTextOption) {
        std::cout << "Output mode is tagged text.\n";
        if (humorESKTokenPosOption)
            throw AHError ("/P cannot be specified together with /T.");
    } else {
        std::cout << "Output mode is symbol listing ";
        if (humorESKTokenPosOption) {
            std::cout << "with HumorESK token positions.\n";
        } else {
            std::cout << "with FiltNER token indices.\n";
        }
    }
    std::cout << "MentionGenerator is " << ((mentionGeneratorDisabled) ? "disabled.": "enabled.") << "\n";
    
    writeProcessing (rootSymbTableName, "HumorESK root symbol table name");

    writeProcessing (filterName, "HumorESK filter name");

    if (!checkOptionalArgument (contextPatternFileName, "ContextPattern file")) {
        std::cout << "Warning: No ContextPattern file specified (/C). Option disabled.\n";
        if (!contextPatternDir.empty ())
            throw AHError ("/D cannot be specified without /C.");
    } else {
	    if (!checkOptionalArgument (contextPatternDir, "ContextPattern directory")) {
		    writeProcessing ("current directory", "ContextPattern directory");
		}
	}

    if (!checkOptionalArgument (unsureArgName, "property name for uncertainity")) {
        std::cout << "Warning: No property name specified for uncertainity (/U)!\n";
    }
}

void ArgHandler::closeInputTextStream () {
    if (inputTextStream.is_open ()) {
        inputTextStream.close ();
    }
}

void ArgHandler::closeOutputTextStream () {
    if (outputTextStream.is_open ()) {
        outputTextStream.close ();
    }
}

ArgHandler::~ArgHandler () {
    // empty
}

std::istream    &ArgHandler::getInputTextStream () {
    if (inputTextStream.is_open ())
        return inputTextStream;
    else
        return std::cin;
}

std::ostream    &ArgHandler::getOutputTextStream () {
    if (outputTextStream.is_open ())
        return outputTextStream;
    else
        return std::cout;
}
