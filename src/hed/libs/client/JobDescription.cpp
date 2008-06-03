#include <cstring>
#include <algorithm>
#include "JobDescription.h"

namespace Arc {
    JobDescriptionError::JobDescriptionError(const std::string& what) : std::runtime_error(what){  }


    // Remove the whitespace characters from the begin and end of the string then return with this "naked" one
    std::string StringManipulator::trim( const std::string original_string ) const {
        std::string whitespaces (" \t\f\v\n\r");
        if (original_string.length() == 0) return original_string;
        unsigned int first = original_string.find_first_not_of( whitespaces );
        unsigned int last = original_string.find_last_not_of( whitespaces );
        if ( first == std::string::npos || last == std::string::npos ) return ""; 
        return original_string.substr( first, last-first+1 );
    }

    // Return the given string in lower case
    std::string StringManipulator::toLowerCase( const std::string original_string ) const {
        std::string retVal = original_string;
        std::transform( original_string.begin(), original_string.end(), retVal.begin(), ::tolower );
        return retVal;
    }

    // Split the given string by the given delimiter and return its parts
    std::vector<std::string> StringManipulator::split( const std::string original_string, const std::string delimiter ) const{
        std::vector<std::string> retVal;
        unsigned int start=0;
        unsigned int end;
        while ( ( end = original_string.find( delimiter, start ) ) != std::string::npos ) {
            retVal.push_back( original_string.substr( start, end-start ) );
            start = end + 1;
        }
        retVal.push_back( original_string.substr( start ) );
        return retVal;
    }

    // Set the sourceString variable of the JobDescriptionOrderer instance
    void JobDescriptionOrderer::setSource( const std::string source ) {
        sourceString = source;
    }

    // A template for the vector sorting function //
    template<class T> class CompareDesc {
        public:
            CompareDesc(){};
            virtual ~CompareDesc(){};
            bool operator()(const T& T1, const T& T2) const {
                return T1.priority > T2.priority;
            }
    }; // End of template Compare

    // Initializing of the candidates vector / collection //
    void JobDescriptionOrderer::determinizeCandidates( std::vector<Candidate>& candidates ) {

        Candidate candidate;

        // JSDL attributes
        //Set defaults//
        candidate.extensions.clear();
        candidate.pattern.clear();
        candidate.negative_pattern.clear();
        candidate.priority = DEFAULT_PRIORITY; //Default value
        //End of setting defaults//
        candidate.typeName = "JSDL";
        candidate.extensions.push_back("jsdl");
        candidate.extensions.push_back("xml");
        candidate.pattern.push_back("<?xml ");
        candidate.pattern.push_back("<!--");
        candidate.pattern.push_back("-->");
        candidate.pattern.push_back("<JobDefinition");
        candidate.pattern.push_back("<JobDescription");
        candidate.pattern.push_back("<JobIdentification");
        candidate.pattern.push_back("<POSIXApplication");
        candidate.pattern.push_back("<Description");
        candidate.pattern.push_back("<Executable");
        candidate.pattern.push_back("<Argument");
        candidate.pattern.push_back("<Input");
        candidate.pattern.push_back("<Output");
        candidate.pattern.push_back("<Error");
        //Save entry  
        candidates.push_back(candidate);
        // End of JSDL

        // RSL attributes
        //Set defaults//
        candidate.extensions.clear();
        candidate.pattern.clear();
        candidate.negative_pattern.clear();
        candidate.priority = DEFAULT_PRIORITY; //Default value
        //End of setting defaults//
        candidate.typeName = "XRSL";
        //  candidate.extensions.push_back("xrsl");
        candidate.pattern.push_back("&(");
        candidate.pattern.push_back("(*");
        candidate.pattern.push_back("*)");
        candidate.pattern.push_back("=");
        candidate.pattern.push_back("(executable");
        candidate.pattern.push_back("(arguments");
        candidate.pattern.push_back("(inputFiles");
        candidate.pattern.push_back("(outputFiles");
        candidate.pattern.push_back("(stdin");
        candidate.pattern.push_back("(stdout");
        candidate.pattern.push_back("(stderr");
        //Save entry  
        candidates.push_back(candidate);
        // End of RSL & xRSL

        //Setting up the JDL attributes
        //Set defaults//
        candidate.extensions.clear();
        candidate.pattern.clear();
        candidate.negative_pattern.clear();
        candidate.priority = DEFAULT_PRIORITY; //Default value
        //End of setting defaults//
        candidate.typeName = "JDL";
        candidate.pattern.push_back("//");
        candidate.pattern.push_back(";");
        candidate.pattern.push_back("{");
        candidate.pattern.push_back("=");
        //  candidate.extensions.push_back("jdl");
        //Save entry  
        candidates.push_back(candidate);
        // End of JDL

    } // End of initiateCandidates

    // Generate a list of the possible source format the order them by the possibility with the help of a scoring algorithm 
    std::vector<Candidate> JobDescriptionOrderer::getCandidateList() {

        std::vector<Candidate> candidates;
        determinizeCandidates( candidates );

        //Examining the filename's extension
        /*
        int pos_of_last_dot = filename.find_last_of('.');
        if ( pos_of_last_dot != std::string::npos ) {
            std::string extension = filename.substr( pos_of_last_dot + 1, filename.length() - pos_of_last_dot -1 );
     
            //To lowercase
            for ( std::string::iterator i = extension.begin(); i != extension.end(); ++i ) *i = tolower(*i);
     
            // Removed //
            // if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\t[COMMENT] Extension: " << extension << std::endl;
        
            for ( std::vector<Candidate>::iterator i = candidates.begin(); i != candidates.end(); i++ ) {
                for ( std::vector<std::string>::const_iterator j = (*i).extensions.begin(); j != (*i).extensions.end(); j++ ) {
                    if ( (*j).compare(extension) == 0 ) {
                        if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tExact extension matching: \"" << extension << "\" to " << (*i).typeName << std::endl;
                        (*i).priority += EXACT_EXTENSION_MATCHING;
                    } else if ( extension.find(*j) != std::string::npos || (*j).find(extension) != std::string::npos ) {
                        if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tPartial extension matching: \"" << extension << "\" to " << (*i).typeName << std::endl;
                        (*i).priority += PARTIAL_EXTENSION_MATCHING;
                    }
                }
            }
        } else { if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]There is no extension... ascpect skipped" << std::endl; }

        // Read the file's content
        std::ifstream jobDescriptionFile;
        std::string jobDescription;
        std::string jobDescriptionWithoutWhitespaces;

        jobDescriptionFile.open( filename.c_str() );
        if ( !jobDescriptionFile ) {
            if ( DEBUGX ) std::cerr << "[JobDescriptionOrderer] Unable to open this file: " << filename << std::endl;
            return false;
        }
        std::string line;
        while(std::getline(jobDescriptionFile,line)) jobDescription += line + "\n";
        jobDescriptionFile.close();
        */

        StringManipulator sm;

        std::string jobDescription = sm.toLowerCase(sourceString);

        //Creating the whitespace-free version of the jobDescription
        std::string jobDescriptionWithoutWhitespaces = jobDescription;
        for ( int i = 0, j; i < jobDescriptionWithoutWhitespaces.length( ); i++ ) {
            if ( jobDescriptionWithoutWhitespaces[i] == ' ' || jobDescriptionWithoutWhitespaces[i] == '\n' || jobDescriptionWithoutWhitespaces[i] == '\t') {
                for ( j = i + 1; j < jobDescriptionWithoutWhitespaces.length( ); j++ ) {
                    if ( jobDescriptionWithoutWhitespaces[j] != ' ' && jobDescriptionWithoutWhitespaces[j] != '\n' && jobDescriptionWithoutWhitespaces[j] != '\t' ) break;
                }
                jobDescriptionWithoutWhitespaces = jobDescriptionWithoutWhitespaces.erase( i, (j - i) ) ;
            }
        }

        //Examining the file's content for patterns
        for ( std::vector<Candidate>::iterator i = candidates.begin(); i != candidates.end(); i++ ) {
            for ( std::vector<std::string>::const_iterator j = (*i).pattern.begin(); j != (*i).pattern.end(); j++ ) {
                if ( jobDescription.find( sm.toLowerCase(*j) ) != std::string::npos ) {
                    if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tText pattern matching: \"" << *j << "\" to " << (*i).typeName << std::endl;
                    (*i).priority += TEXT_PATTERN_MATCHING;
                } else {
                    if ( jobDescriptionWithoutWhitespaces.find( sm.toLowerCase(*j) ) != std::string::npos ) {
                        if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tText pattern matching (just with removed whitespaces) : \"" << *j << "\" to " << (*i).typeName << std::endl;
                        (*i).priority += TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES;
                    }
                }
            }
            for ( std::vector<std::string>::const_iterator j = (*i).negative_pattern.begin(); j != (*i).negative_pattern.end(); j++ ) {
                if ( jobDescription.find( sm.toLowerCase(*j) ) != std::string::npos ) {
                    if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tNegative text pattern matching: \"" << *j << "\" to " << (*i).typeName << std::endl;
                    (*i).priority += NEGATIVE_TEXT_PATTERN_MATCHING;
                } else {
                    if ( jobDescriptionWithoutWhitespaces.find( sm.toLowerCase(*j) ) != std::string::npos ) {
                        if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tNegative text pattern matching (just with removed whitespaces): \"" << *j << "\" to " << (*i).typeName << std::endl;
                        (*i).priority += NEGATIVE_TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES;
                    }
                }
            }
        }

        // Sort candidates by priority
        std::sort(candidates.begin(), candidates.end(), CompareDesc<Candidate>() );

        return candidates;
    }

    // JobDescription default constructor
    JobDescription::JobDescription() {
        sourceFormat = "";
        sourceString = "";
    }

    // Set the sourceString variable of the JobDescription instance and parse it with the right parser
    void JobDescription::setSource( const std::string source ) throw(JobDescriptionError) {
        sourceFormat = "";
        sourceString = source;
        
        // Initialize the necessary variables
        resetJobTree();

        // Parsing the source string depending on the Orderer's decision
        //
        // Get the candidate list of formats in the proper order
        if ( sourceString.empty() || sourceString.length() == 0 ) throw JobDescriptionError("There is nothing in the source. Cannot generate any product.");

        jdOrderer.setSource( sourceString );
        std::vector<Candidate> candidates = jdOrderer.getCandidateList();

        // Try to parse the input string in the right order until it once success
        // (If not then just reset the jobTree variable and see the next one)
        for (std::vector<Candidate>::const_iterator it = candidates.begin(); it < candidates.end(); it++) {
            if ( sm.toLowerCase( (*it).typeName ) == "xrsl" ) {
                if ( VERBOSEX ) std::cerr << "[JobDescription] Try to parse as XRSL" << std::endl;
                XRSLParser parser;
                if ( parser.parse( jobTree, sourceString ) ) {
                    sourceFormat = "xrsl";
                    break;
                }
                //else
                resetJobTree();
            } else if ( sm.toLowerCase( (*it).typeName ) == "jsdl" ) {
                if ( VERBOSEX ) std::cerr << "[JobDescription] Try to parse as JSDL" << std::endl;
                JSDLParser parser;
                if ( parser.parse( jobTree, sourceString ) ) {
                    sourceFormat = "jsdl";
                    break;
                }
                //else
                resetJobTree();
            } else if ( sm.toLowerCase( (*it).typeName ) == "jdl" ) {
                if ( VERBOSEX ) std::cerr << "[JobDescription] Try to parse as JDL" << std::endl;
                JDLParser parser;
                if ( parser.parse( jobTree, sourceString ) ) {
                    sourceFormat = "jdl";
                    break;
                }
                //else
                resetJobTree();
            }
        }
        if (sourceFormat.length() == 0) throw JobDescriptionError("The parsing of the source string was unsuccessful.");
    }

    // Create a new Arc::XMLNode with a single root element and out it into the JobDescription's jobTree variable
    void JobDescription::resetJobTree() {
        (Arc::XMLNode ( "<JobDefinition />" )).New( jobTree );
    }

    // Returns with the jobTree XMLNode or throws an error if it's emtpy
    Arc::XMLNode JobDescription::getXML() throw(JobDescriptionError) {
        if (!(bool)(jobTree)) throw JobDescriptionError("There is no valid job inner-representation available.");
        return jobTree;
    }
    
    // Generate the output in the requested format
    void JobDescription::getProduct( std::string& product, std::string format ) throw(JobDescriptionError) {
        // Initialize the necessary variables
        product = "";

        // Generate the output text with the right parser class
        if ( !this->isValid() ) throw JobDescriptionError("There is no successfully parsed source");
        if ( sm.toLowerCase( format ) == sm.toLowerCase( sourceFormat ) ) {
            product = sourceString;
            return;
        }
        if ( sm.toLowerCase( format ) == "jdl" ) {
            if ( VERBOSEX ) std::cerr << "[JobDescription] Generate JDL output" << std::endl;
            JDLParser parser;
            if ( !parser.parse( jobTree, product ) ) throw JobDescriptionError("Generating " + format + " output was unsuccessful");
            return;
        } else if ( sm.toLowerCase( format ) == "xrsl" ) {
            if ( VERBOSEX ) std::cerr << "[JobDescription] Generate XRSL output" << std::endl;
            XRSLParser parser;
            if ( !parser.parse( jobTree, product ) ) throw JobDescriptionError("Generating " + format + " output was unsuccessful");
            return;
        } else if ( sm.toLowerCase( format ) == "jsdl" ) {
            if ( VERBOSEX ) std::cerr << "[JobDescription] Generate JSDL output" << std::endl;
            JSDLParser parser;
            if ( !parser.getProduct( jobTree, product ) ) throw JobDescriptionError("Generating " + format + " output was unsuccessful");
            return;
        } else {
            throw JobDescriptionError("Unknown output format: " + format);
        }
    }

    bool JSDLParser::parse( Arc::XMLNode& jobTree, const std::string source ) {
        //schema verification would be needed
        (Arc::XMLNode ( source )).New( jobTree );
        return bool( jobTree );
    }

    bool JSDLParser::getProduct( const Arc::XMLNode& jobTree, std::string& product ) {
        Arc::XMLNode jobDescription = jobTree["JobDescription"];
        if ( bool( jobDescription ) ) {
            Arc::XMLNode outputTree( "<JobDefinition />" );
            outputTree.NewChild( jobDescription );
            outputTree.GetDoc( product );
            return true;
        } else {
            if ( DEBUGX ) std::cerr << "[JSDLParser] Job inner representation's root element has not found." << std::endl;
            return false;
        }
    }

    std::string XRSLParser::simpleXRSLvalue(std::string attributeValue) {
        std::string quote_mark = "";
        std::string first_char_of_attributeValue = attributeValue.substr(0,1);

        //Ruler character examination
        if (first_char_of_attributeValue == "^") quote_mark = attributeValue.substr(0,2);
        else if (first_char_of_attributeValue == "\"") quote_mark = "\"";
        else if (first_char_of_attributeValue == "'") quote_mark = "'";
  
        if (quote_mark != "") attributeValue = attributeValue.substr(attributeValue.find_first_of(quote_mark)+quote_mark.length(),
            attributeValue.find_last_of(quote_mark.substr(0,1))-quote_mark.length()-attributeValue.find_first_of(quote_mark) );
  
        return attributeValue;
    }

    std::vector<std::string> XRSLParser::listXRSLvalue(std::string attributeValue) {

        std::string whitespaces (" \t\f\v\n\r");

        std::vector<std::string> attributes;
        unsigned int start_pointer;
        for ( unsigned int end_pointer = start_pointer = attributeValue.find_first_not_of(whitespaces); start_pointer != std::string::npos; ) {
            std::string next_char = attributeValue.substr(start_pointer,1);
            //If we have a custom quotation mark
            if ( next_char == "^" ) {
                std::string other_half = attributeValue.substr(start_pointer+1,1);
                end_pointer = attributeValue.find_first_of("^", start_pointer+1);
                while ( attributeValue.substr(end_pointer+1,1) != other_half ) {
                    end_pointer = attributeValue.find_first_of("^", end_pointer+1);
                }
                attributes.push_back( attributeValue.substr(start_pointer+2, end_pointer-start_pointer-2) );
                start_pointer = attributeValue.find_first_not_of(whitespaces, end_pointer+2);
            } else if ( next_char == "\"" || next_char == "'" ) {
                end_pointer = attributeValue.find_first_of( next_char, start_pointer+1 );
                while ( attributeValue.substr(end_pointer-1,1) == "\\" ) {
                    end_pointer = attributeValue.find_first_of( next_char, end_pointer+1);
                }
                attributes.push_back( attributeValue.substr(start_pointer+1, end_pointer-start_pointer-1) );
                start_pointer = attributeValue.find_first_not_of(whitespaces, end_pointer+1);
            } else {
                end_pointer = attributeValue.find_first_of(whitespaces, start_pointer+1);
                attributes.push_back( attributeValue.substr(start_pointer, end_pointer-start_pointer) );
                start_pointer = attributeValue.find_first_not_of(whitespaces, end_pointer);
            }
        }

        return attributes;
    }

    std::vector< std::vector<std::string> > XRSLParser::doubleListXRSLvalue(std::string attributeValue ) {

        std::vector< std::vector<std::string> > new_attributeValue;
        std::string whitespaces (" \t\f\v\n\r");
        std::string letters ("abcdefghijklmnopqrstuvwxyz");

        unsigned int outer_start_pointer;
        unsigned int outer_end_pointer = 0;
        for ( outer_start_pointer = attributeValue.find_first_of("(", outer_end_pointer); outer_start_pointer != std::string::npos; outer_start_pointer = attributeValue.find_first_of("(", outer_end_pointer) ) {
            outer_end_pointer = attributeValue.find_first_of(")", outer_start_pointer);

            std::string inner_attributeValue = attributeValue.substr(outer_start_pointer+1,outer_end_pointer-outer_start_pointer-1);
            std::vector<std::string> attributes;
            unsigned int start_pointer;
            for ( unsigned int end_pointer = start_pointer = inner_attributeValue.find_first_not_of(whitespaces); start_pointer != std::string::npos; ) {
                std::string next_char = inner_attributeValue.substr(start_pointer,1);
                //If we have a custom quotation mark
                if ( next_char == "^" ) {
                    std::string other_half = inner_attributeValue.substr(start_pointer+1,1);
                    end_pointer = inner_attributeValue.find_first_of("^", start_pointer+1);
                    while ( inner_attributeValue.substr(end_pointer+1,1) != other_half ) {
                        end_pointer = inner_attributeValue.find_first_of("^", end_pointer+1);
                    }
                    attributes.push_back( inner_attributeValue.substr(start_pointer+2, end_pointer-start_pointer-2) );
                    start_pointer = inner_attributeValue.find_first_not_of(whitespaces, end_pointer+2);
                } else if ( next_char == "\"" || next_char == "'" ) {
                    end_pointer = inner_attributeValue.find_first_of( next_char, start_pointer+1 );
                    while ( end_pointer > 0 && inner_attributeValue.substr(end_pointer-1,1) == "\\" ) {
                        end_pointer = inner_attributeValue.find_first_of( next_char, end_pointer+1);
                    }
                    attributes.push_back( inner_attributeValue.substr(start_pointer+1, end_pointer-start_pointer-1) );
                    start_pointer = inner_attributeValue.find_first_not_of(whitespaces, end_pointer+1);
                } else {
                    end_pointer = inner_attributeValue.find_first_of(whitespaces, start_pointer+1);
                    if ( end_pointer != std::string::npos ) attributes.push_back( inner_attributeValue.substr(start_pointer, end_pointer-start_pointer) );
                    else attributes.push_back( inner_attributeValue.substr(start_pointer) );
                    start_pointer = inner_attributeValue.find_first_not_of(whitespaces, end_pointer);
                }
            }
            new_attributeValue.push_back( attributes );
        }
        return new_attributeValue;
    }

    bool XRSLParser::handleXRSLattribute(std::string attributeName, std::string attributeValue, Arc::XMLNode& jobTree) {

        // To do the attributes name case-insensitive do them lowercase and remove the quotiation marks
        attributeName = simpleXRSLvalue( sm.toLowerCase( attributeName ) );  

        if ( attributeName == "executable" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Executable");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["Executable"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "arguments") {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            std::string new_attributeValue;
            std::vector<std::string> attributes = listXRSLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=attributes.begin(); it<attributes.end(); it++) {
                Arc::XMLNode attribute = jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Argument");
                attribute = (*it);
            }
            return true;
        } else if ( attributeName == "stdin" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Input");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["Input"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stdout" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Output");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["Output"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stderr" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Error");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["Error"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "inputfiles" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            std::vector< std::vector<std::string> > attributesDoubleVector = doubleListXRSLvalue( attributeValue );
            for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin(); it<attributesDoubleVector.end(); it++) {
                Arc::XMLNode ds = jobTree["JobDescription"].NewChild("DataStaging");
                Arc::XMLNode fn = ds.NewChild("FileName");
                fn = (*it)[0];
                if ( (*it)[1] != "" ) {
                    Arc::XMLNode source = ds.NewChild("Source");
                    Arc::XMLNode uri = source.NewChild("URI");
                    uri = (*it)[1];
                }
            }
            return true;
        } else if ( attributeName == "executables" ) {
            Arc::NS nsList;
            nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
            nsList.insert(std::pair<std::string, std::string>("jsdlPOSIX","http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
            nsList.insert(std::pair<std::string, std::string>("jsdlARC","http://www.nordugrid.org/ws/schemas/jsdl-arc"));

            Arc::XMLNodeList dataStagings = jobTree.XPathLookup( (std::string) "//DataStaging", nsList);

            std::vector<std::string> attributes = listXRSLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=attributes.begin(); it<attributes.end(); it++) {
                for (std::list<Arc::XMLNode>::const_iterator it2 = dataStagings.begin(); it2 != dataStagings.end(); it2++) {
                    if ( ( (std::string) (*it2)["FileName"]) == (*it) ) (*it2)["IsExecutable"] = "true";
                }
            }
            return true;
        } else if ( attributeName == "cache" ) {
            if ( !bool( jobTree["XRSLDescription"] ) ) jobTree.NewChild("XRSLDescription");
            if ( !bool( jobTree["XRSLDescription"]["Cache"] ) ) jobTree["XRSLDescription"].NewChild("Cache");
            jobTree["XRSLDescription"]["Cache"] = simpleXRSLvalue( attributeValue );
        } else if ( attributeName == "outputfiles" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            std::vector< std::vector<std::string> > attributesDoubleVector = doubleListXRSLvalue( attributeValue );
            for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin(); it<attributesDoubleVector.end(); it++) {
                Arc::XMLNode ds = jobTree["JobDescription"].NewChild("DataStaging");
                Arc::XMLNode fn = ds.NewChild("FileName");
                fn = (*it)[0];
                if ( (*it)[1] != "" ) {
                    Arc::XMLNode source = ds.NewChild("Target");
                    Arc::XMLNode uri = source.NewChild("URI");
                    uri = (*it)[1];
                }
            }
            return true;
        } else if ( attributeName == "queue" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            if ( !bool( jobTree["JobDescription"]["Resources"]["CandidateTarget"] ) ) jobTree["JobDescription"]["Resources"].NewChild("CandidateTarget");
            jobTree["JobDescription"]["Resources"]["CandidateTarget"].NewChild("QueueName");
            jobTree["JobDescription"]["Resources"]["CandidateTarget"]["QueueName"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "starttime" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            jobTree["JobDescription"].NewChild("ProcessingStartTime");
            jobTree["JobDescription"]["ProcessingStartTime"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "lifetime" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            jobTree["JobDescription"]["Resources"].NewChild("SessionLifeTime");
            jobTree["JobDescription"]["Resources"]["SessionLifeTime"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "cputime" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            jobTree["JobDescription"]["Resources"].NewChild("TotalCpuTime");
            jobTree["JobDescription"]["Resources"]["TotalCpuTime"] = simpleXRSLvalue( attributeValue );
            jobTree["JobDescription"]["Resources"].NewChild("IndividualCpuTime");
            jobTree["JobDescription"]["Resources"]["IndividualCpuTime"] = simpleXRSLvalue( attributeValue );
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("CPUTimeLimit");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["CPUTimeLimit"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "walltime" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("WallTimeLimit");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["WallTimeLimit"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "gridtime" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            jobTree["JobDescription"]["Resources"].NewChild("GridTime");
            jobTree["JobDescription"]["Resources"]["GridTime"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "benchmarks" ) {
            if ( !bool( jobTree["XRSLDescription"] ) ) jobTree.NewChild("XRSLDescription");
            jobTree["XRSLDescription"].NewChild("Benchmarks");
            jobTree["XRSLDescription"]["Benchmarks"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "memory" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            jobTree["JobDescription"]["Resources"].NewChild("TotalPhysicalMemory");
            jobTree["JobDescription"]["Resources"]["TotalPhysicalMemory"] = simpleXRSLvalue( attributeValue );
            jobTree["JobDescription"]["Resources"].NewChild("IndividualPhysicalMemory");
            jobTree["JobDescription"]["Resources"]["IndividualPhysicalMemory"] = simpleXRSLvalue( attributeValue );
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("MemoryLimit");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["MemoryLimit"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "disk" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            jobTree["JobDescription"]["Resources"].NewChild("TotalDiskSpace");
            jobTree["JobDescription"]["Resources"]["TotalDiskSpace"] = simpleXRSLvalue( attributeValue );
            jobTree["JobDescription"]["Resources"].NewChild("IndividualDiskSpace");
            jobTree["JobDescription"]["Resources"]["IndividualDiskSpace"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "runtimeenvironment" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            jobTree["JobDescription"]["Resources"].NewChild("RunTimeEnvironment");
            jobTree["JobDescription"]["Resources"]["RunTimeEnvironment"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "middleware" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            jobTree["JobDescription"]["Resources"].NewChild("Middleware");
            jobTree["JobDescription"]["Resources"]["Middleware"] = simpleXRSLvalue( attributeValue );
            return true;    
        } else if ( attributeName == "opsys" ) {
            if ( !bool( jobTree["XRSLDescription"] ) ) jobTree.NewChild("XRSLDescription");
            jobTree["XRSLDescription"].NewChild("OPSys");
            jobTree["XRSLDescription"]["OPSys"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "join" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            jobTree["JobDescription"].NewChild("JoinOutputs");
            jobTree["JobDescription"]["JoinOutputs"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "gmlog" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["LocalLogging"] ) ) jobTree["JobDescription"].NewChild("LocalLogging");
            jobTree["JobDescription"]["LocalLogging"].NewChild("Directory");
            jobTree["JobDescription"]["LocalLogging"]["Directory"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "jobname" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["JobIdentification"] ) ) jobTree["JobDescription"].NewChild("JobIdentification");
            jobTree["JobDescription"]["JobIdentification"].NewChild("JobName");
            jobTree["JobDescription"]["JobIdentification"]["JobName"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "ftpthreads" ) {
            if ( !bool( jobTree["XRSLDescription"] ) ) jobTree.NewChild("XRSLDescription");
            jobTree["XRSLDescription"].NewChild("FTPThreads");
            jobTree["XRSLDescription"]["FTPThreads"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "acl" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["AccessControl"] ) ) jobTree["JobDescription"].NewChild("AccessControl");
            Arc::XMLNode type = jobTree["JobDescription"]["AccessControl"].NewChild("Type");
            type = "GACL";
            Arc::XMLNode content = jobTree["JobDescription"]["AccessControl"].NewChild("Content");
            content = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "cluster" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            if ( !bool( jobTree["JobDescription"]["Resources"]["CandidateTarget"] ) ) jobTree["JobDescription"]["Resources"].NewChild("CandidateTarget");
            jobTree["JobDescription"]["Resources"]["CandidateTarget"].NewChild("HostName");
            jobTree["JobDescription"]["Resources"]["CandidateTarget"]["HostName"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "notify" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Notify"] ) ) jobTree["JobDescription"].NewChild("Notify");
            jobTree["JobDescription"]["Notify"].NewChild("Type");
            jobTree["JobDescription"]["Notify"]["Type"] = "Email";
            return true;
        } else if ( attributeName == "replicacollection" ) {
            if ( !bool( jobTree["XRSLDescription"] ) ) jobTree.NewChild("XRSLDescription");
            jobTree["XRSLDescription"].NewChild("ReplicaCollection");
            jobTree["XRSLDescription"]["ReplicaCollection"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "rerun" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Reruns"] ) ) jobTree["JobDescription"].NewChild("Reruns");
            jobTree["JobDescription"]["Reruns"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "architecture" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            if ( !bool( jobTree["JobDescription"]["Resources"]["CPUArchitecture"] ) ) jobTree["JobDescription"]["Resources"].NewChild("CPUArchitecture");
            jobTree["JobDescription"]["Resources"]["CPUArchitecture"].NewChild("CPUArchitectureName");
            jobTree["JobDescription"]["Resources"]["CPUArchitecture"]["CPUArchitectureName"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "nodeaccess" ) {
            if ( !bool( jobTree["XRSLDescription"] ) ) jobTree.NewChild("XRSLDescription");
            jobTree["XRSLDescription"].NewChild("NodeAccess");
            jobTree["XRSLDescription"]["NodeAccess"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "dryrun" ) {
            if ( !bool( jobTree["XRSLDescription"] ) ) jobTree.NewChild("XRSLDescription");
            jobTree["XRSLDescription"].NewChild("DryRun");
            jobTree["XRSLDescription"]["DryRun"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "rsl_substitution" ) {
            if ( !bool( jobTree["XRSLDescription"] ) ) jobTree.NewChild("XRSLDescription");
            jobTree["XRSLDescription"].NewChild("RSLSubstitution");
            jobTree["XRSLDescription"]["RSLSubstitution"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "environment" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            Arc::XMLNode posix_application = jobTree["JobDescription"]["Application"]["POSIXApplication"];
            std::vector< std::vector<std::string> > attributesDoubleVector = doubleListXRSLvalue( attributeValue );
            for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin(); it<attributesDoubleVector.end(); it++) {
                Arc::XMLNode envir = posix_application.NewChild("Environment");
                Arc::XMLNode attribute = envir.NewAttribute("name");
                attribute = (*it)[0];
                envir = (*it)[1];
            }
            return true;
        } else if ( attributeName == "count" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            jobTree["JobDescription"]["Resources"].NewChild("TotalCpuCount");
            jobTree["JobDescription"]["Resources"]["TotalCpuCount"] = simpleXRSLvalue( attributeValue );
            jobTree["JobDescription"]["Resources"].NewChild("IndividualCpuCount");
            jobTree["JobDescription"]["Resources"]["IndividualCpuCount"] = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "jobreport" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["RemoteLogging"] ) ) jobTree["JobDescription"].NewChild("RemoteLogging");
            jobTree["JobDescription"]["RemoteLogging"].NewChild("URL");
            jobTree["JobDescription"]["RemoteLogging"]["URL"] = simpleXRSLvalue( attributeValue );
            return true;    
        } else if ( attributeName == "credentialserver" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["CredentialServer"] ) ) jobTree["JobDescription"].NewChild("CredentialServer");
            jobTree["JobDescription"]["CredentialServer"].NewChild("URL");
            jobTree["JobDescription"]["CredentialServer"]["URL"] = simpleXRSLvalue( attributeValue );
            return true;
        }
        if ( DEBUGX ) std::cerr << "[XRSLParser] Unknown XRSL attribute: " << attributeName << std::endl;
        return false;
    }

    bool XRSLParser::parse( Arc::XMLNode& jobTree, const std::string source ) {
        //First step: trim the text
        //Second step: distinguis the different attributes & convert string to C char array
        int depth = 0;
        char xrsl_text[source.length()+1];
        strcpy( xrsl_text, (source.substr( source.find_first_of("&")+1, source.length()-source.find_first_of("&")-1)).c_str() );
        bool comment = false;
        std::string actual_argument = "";
  
        for (unsigned int pos=0; pos < strlen(xrsl_text)-1; pos++) {
            char next_char = xrsl_text[pos];
            if ( comment && next_char == ')' && xrsl_text[pos-1] == '*' ) comment = false;
            else if ( !comment ) {
                if ( next_char == '*' && xrsl_text[pos-1] == '(' ) {
                    comment = true;
                    depth--;
                } else if ( next_char == '(' ) {
                    if ( depth == 0 ) actual_argument.clear();
                    else actual_argument += next_char;
                    depth++;
                } else if ( next_char == ')' ) {
                    depth--;
                    if ( depth == 0 ) {
                        //Third step: You can handle the different attribute blocks here.
                        //Trim the unnecassary whitespaces
                        std::string wattr = sm.trim( actual_argument );
                        //Split the bracket's content by the equal sign & trim the two half
                        unsigned int eqpos = wattr.find_first_of("=");
                        if ( eqpos == std::string::npos ) {
                            if ( DEBUGX ) std::cerr << "[XRSLParser] XRSL Syntax error (attribute declaration without equal sign)" << std::endl;
                            return false;
                        }
                        if ( !handleXRSLattribute( sm.trim( wattr.substr(0,eqpos) ) , sm.trim( wattr.substr(eqpos+1) ) , jobTree) ) return false;
                    } else actual_argument += next_char;
                } else {
                    actual_argument += next_char;
                }
            }
        }
  
        if (depth != 0 ) {
            if ( DEBUGX ) std::cerr << "[XRSLParser] XRSL Syntax error (bracket mistake)" << std::endl;
            return false;
        }
        return true;
    }

    //This feature is not supported yet
    bool XRSLParser::getProduct( const Arc::XMLNode& jobTree, std::string& product ) {
        if ( DEBUGX ) std::cerr << "[XRSLParser] Converting to XRSL - This feature is not supported yet" << std::endl;
        return false;
    }

    bool JDLParser::splitJDL(std::string original_string, std::vector<std::string>& lines) {

        // Clear the return variable
        lines.clear();
      
        char jdl_text[original_string.length()+1];
        strcpy( jdl_text, original_string.c_str() );
      
        unsigned int start_pos=0;
        bool quotation = false;
        std::vector<char> stack;
        std::string actual_line;
  
        for (int i=0; i<strlen(jdl_text)-1; i++) {
            // Looking for control character marks the line end
            if ( jdl_text[i] == ';' && !quotation && stack.empty() ) {
                lines.push_back( actual_line );
                actual_line.clear();
                continue;
            }
            // Determinize the quotations
            if ( jdl_text[i] == '"' ) {
                if ( !quotation ) quotation = true;
                else if ( jdl_text[i-1] != '\\' ) quotation = false;
            }
            if ( !quotation ) {
                if ( jdl_text[i] == '{' || jdl_text[i] == '[' ) stack.push_back( jdl_text[i] );
                if ( jdl_text[i] == '}' ) {
                    if ( stack.back() == '{') stack.pop_back();
                    else return false;
                }
                if ( jdl_text[i] == ']' ) {
                    if ( stack.back() == '[') stack.pop_back();
                    else return false;
                }
            }
            actual_line += jdl_text[i];
        }
        return true;
    }

    std::string JDLParser::simpleJDLvalue(std::string attributeValue) {
        std::string whitespaces (" \t\f\v\n\r");
        unsigned int last_pos = attributeValue.find_last_of( "\"" );
        // If the text is not between quotation marks, then return with the original form
        if (attributeValue.substr( attributeValue.find_first_not_of( whitespaces ), 1 ) != "\"" || last_pos == std::string::npos )
            return sm.trim( attributeValue );
        // Else remove the marks and return with the quotation's content
        else 
            return attributeValue.substr( attributeValue.find_first_of( "\"" )+1,  last_pos - attributeValue.find_first_of( "\"" ) -1 );
    }

    std::vector<std::string> JDLParser::listJDLvalue( std::string attributeValue ) {
        std::vector<std::string> elements;
        unsigned int first_bracket = attributeValue.find_first_of( "{" );
        if ( first_bracket == std::string::npos ) {
            elements.push_back( simpleJDLvalue( attributeValue ) );
            return elements;
        }
        unsigned int last_bracket = attributeValue.find_last_of( "}" );
        if ( last_bracket == std::string::npos ) {
            elements.push_back( simpleJDLvalue( attributeValue ) );
            return elements;
        }
        attributeValue = attributeValue.substr(first_bracket+1, last_bracket - first_bracket - 1 );
        std::vector<std::string> listElements = sm.split( attributeValue, "," );
        for (std::vector<std::string>::const_iterator it=listElements.begin(); it<listElements.end(); it++) {
            elements.push_back( simpleJDLvalue( *it ) );
        }
        return elements;
    }

    bool JDLParser::handleJDLattribute(std::string attributeName, std::string attributeValue, Arc::XMLNode& jobTree) {

        // To do the attributes name case-insensitive do them lowercase and remove the quotiation marks
        attributeName = sm.toLowerCase( attributeName );
        if ( attributeName == "type" ) {
            std::string value = sm.toLowerCase( simpleJDLvalue( attributeValue ) );
            if ( value == "job" ) return true;
            if ( value == "dag" ) {
                if ( DEBUGX ) std::cerr << "[JDLParser] This kind of JDL decriptor is not supported yet: " << value << std::endl;
                return false; // This kind of JDL decriptor is not supported yet
            }
            if ( value == "collection" ) {
                if ( DEBUGX ) std::cerr << "[JDLParser] This kind of JDL decriptor is not supported yet: " << value << std::endl;
                return false; // This kind of JDL decriptor is not supported yet
            }
            if ( DEBUGX ) std::cerr << "[JDLParser] Attribute name: " << attributeName << ", has unknown value: " << value << std::endl;
            return false; // Unknown attribute value - error
        } else if ( attributeName == "jobtype") {
            return true; // Skip this attribute
        } else if ( attributeName == "executable" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Executable");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["Executable"] = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "arguments" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            std::string value = simpleJDLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            for ( std::vector<std::string>::const_iterator it = parts.begin(); it < parts.end(); it++ ) {
                Arc::XMLNode attribute = jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Attribute");
                attribute = (*it);
            }
            return true;
        } else if ( attributeName == "stdinput" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Input");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["Input"] = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stdoutput" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Output");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["Output"] = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stderror" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            jobTree["JobDescription"]["Application"]["POSIXApplication"].NewChild("Error");
            jobTree["JobDescription"]["Application"]["POSIXApplication"]["Error"] = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "inputsandbox" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            std::vector<std::string> inputfiles = listJDLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=inputfiles.begin(); it<inputfiles.end(); it++) {
                Arc::XMLNode ds = jobTree["JobDescription"].NewChild("DataStaging");
                Arc::XMLNode fn = ds.NewChild("FileName");
                fn = (*it);
                Arc::XMLNode source = ds.NewChild("Source");
                Arc::XMLNode uri = source.NewChild("URI");
                uri = (*it);
            }
            return true;
        } else if ( attributeName == "inputsandboxbaseuri" ) {
            std::string value = simpleJDLvalue( attributeValue );
            Arc::NS nsList;
            nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
            nsList.insert(std::pair<std::string, std::string>("jsdlPOSIX","http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
            nsList.insert(std::pair<std::string, std::string>("jsdlARC","http://www.nordugrid.org/ws/schemas/jsdl-arc"));
            Arc::XMLNodeList sources = jobTree.XPathLookup( (std::string) "//DataStaging/Source/URI", nsList);

            for (std::list<Arc::XMLNode>::iterator it = sources.begin(); it != sources.end(); it++) {
                (*it).Set( value + "/" + (std::string) (*it) );
            }
            return true;
        } else if ( attributeName == "outputsandbox" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            std::vector<std::string> inputfiles = listJDLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=inputfiles.begin(); it<inputfiles.end(); it++) {
                Arc::XMLNode ds = jobTree["JobDescription"].NewChild("DataStaging");
                Arc::XMLNode fn = ds.NewChild("FileName");
                fn = (*it);
                Arc::XMLNode source = ds.NewChild("Target");
                Arc::XMLNode uri = source.NewChild("URI");
                uri = (*it);
            }
            return true;
        } else if ( attributeName == "outputsandboxdesturi" ) {
            std::vector< std::string > value = listJDLvalue( attributeValue );
            Arc::NS nsList;
            nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
            nsList.insert(std::pair<std::string, std::string>("jsdlPOSIX","http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
            nsList.insert(std::pair<std::string, std::string>("jsdlARC","http://www.nordugrid.org/ws/schemas/jsdl-arc"));
            Arc::XMLNodeList targets = jobTree.XPathLookup( (std::string) "//DataStaging/Target", nsList);
            Arc::XMLNodeList::iterator xml_it = targets.begin();
            for (std::vector< std::string >::const_iterator it = value.begin(); it != value.end(); it++ ) {
                (*xml_it)["URI"].Set((std::string)(*it));
                xml_it++;
            }
            return true;
        } else if ( attributeName == "outputsandboxbaseuri" ) {
            std::string value = simpleJDLvalue( attributeValue );
            Arc::NS nsList;
            nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
            nsList.insert(std::pair<std::string, std::string>("jsdlPOSIX","http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
            nsList.insert(std::pair<std::string, std::string>("jsdlARC","http://www.nordugrid.org/ws/schemas/jsdl-arc"));
            Arc::XMLNodeList targets = jobTree.XPathLookup( (std::string) "//DataStaging/Target/URI", nsList);
            
            for (std::list<Arc::XMLNode>::iterator it = targets.begin(); it != targets.end(); it++) {
                (*it).Set( value + "/" + (std::string) (*it) );
            }
            return true;
        } else if ( attributeName == "batchsystem" ) {
            if ( !bool( jobTree["JDLDescription"] ) ) jobTree.NewChild("JDLDescription");
            if ( !bool( jobTree["JDLDescription"]["BatchSystem"] ) ) jobTree["JDLDescription"].NewChild("BatchSystem");
            jobTree["JDLDescription"]["BatchSystem"] = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "prologue" ) {
            if ( !bool( jobTree["JDLDescription"] ) ) jobTree.NewChild("JDLDescription");
            if ( !bool( jobTree["JDLDescription"]["Prologue"] ) ) jobTree["JDLDescription"].NewChild("Prologue");
            jobTree["JDLDescription"]["Prologue"].NewChild("Executable");
            jobTree["JDLDescription"]["Prologue"]["Executable"] = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "prologuearguments" ) {
            if ( !bool( jobTree["JDLDescription"] ) ) jobTree.NewChild("JDLDescription");
            if ( !bool( jobTree["JDLDescription"]["Prologue"] ) ) jobTree["JDLDescription"].NewChild("Prologue");
            std::string value = simpleJDLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            for ( std::vector<std::string>::const_iterator it = parts.begin(); it < parts.end(); it++ ) {
                Arc::XMLNode attribute = jobTree["JDLDescription"]["Prologue"].NewChild("Attribute");
                attribute = (*it);
            }
            return true;
        } else if ( attributeName == "epilogue" ) {
            if ( !bool( jobTree["JDLDescription"] ) ) jobTree.NewChild("JDLDescription");
            if ( !bool( jobTree["JDLDescription"]["Epilogue"] ) ) jobTree["JDLDescription"].NewChild("Epilogue");
            jobTree["JDLDescription"]["Epilogue"].NewChild("Executable");
            jobTree["JDLDescription"]["Epilogue"]["Executable"] = simpleJDLvalue( attributeValue );
            return true;    
        } else if ( attributeName == "epiloguearguments" ) {
            if ( !bool( jobTree["JDLDescription"] ) ) jobTree.NewChild("JDLDescription");
            if ( !bool( jobTree["JDLDescription"]["Epilogue"] ) ) jobTree["JDLDescription"].NewChild("Epilogue");
            std::string value = simpleJDLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            for ( std::vector<std::string>::const_iterator it = parts.begin(); it < parts.end(); it++ ) {
                Arc::XMLNode attribute = jobTree["JDLDescription"]["Epilogue"].NewChild("Attribute");
                attribute = (*it);
            }
            return true;
        } else if ( attributeName == "allowzippedisb" ) {
            if ( sm.toLowerCase( simpleJDLvalue( attributeValue ) ) == "true" ) {
                if ( !bool( jobTree["JDLDescription"] ) ) jobTree.NewChild("JDLDescription");
                if ( !bool( jobTree["JDLDescription"]["ZippedISB"] ) ) jobTree["JDLDescription"].NewChild("ZippedISB");
            }
            return true;
        } else if ( attributeName == "zippedisb" ) {
            if ( bool( jobTree["JDLDescription"]["ZippedISB"] ) ) jobTree["JDLDescription"]["ZippedISB"] = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "expirytime" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Resources"] ) ) jobTree["JobDescription"].NewChild("Resources");
            jobTree["JobDescription"]["Resources"].NewChild("SessionLifeTime");
    
            int lifeTimeSeconds = atoi( simpleJDLvalue( attributeValue ).c_str() );
            int nowSeconds = time(NULL);
            std::stringstream ss;
            ss << lifeTimeSeconds - nowSeconds;
            ss >> attributeValue;
    
            jobTree["JobDescription"]["Resources"]["SessionLifeTime"] = attributeValue;
            return true;
        } else if ( attributeName == "environment" ) {
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Application"] ) ) jobTree["JobDescription"].NewChild("Application");
            if ( !bool( jobTree["JobDescription"]["Application"]["POSIXApplication"] ) ) jobTree["JobDescription"]["Application"].NewChild("POSIXApplication");
            Arc::XMLNode posix_application = jobTree["JobDescription"]["Application"]["POSIXApplication"];
    
            std::vector<std::string> variables = listJDLvalue( attributeValue );
    
            for (std::vector<std::string>::const_iterator it = variables.begin(); it < variables.end(); it++) {
                unsigned int equal_pos = (*it).find_first_of( "=" );
                if ( equal_pos != std::string::npos ) {
                    Arc::XMLNode envir = posix_application.NewChild("Environment");
                    Arc::XMLNode attribute = envir.NewAttribute("name");
                    attribute = (*it).substr( 0,equal_pos );
                    envir = (*it).substr( equal_pos+1, std::string::npos );
                } else {
                    if ( DEBUGX ) std::cerr << "[JDLParser] Environment variable has been defined without any equal sign." << std::endl;
                    return false;
                }
            }
            return true;
        } else if ( attributeName == "perusalfileenable" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "perusaltimeinterval" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "perusalfiledesturi" ) {
            // Not supported yet
            return true;    
        } else if ( attributeName == "inputdata" ) {
            // TODO
            return true;
        } else if ( attributeName == "outputdata" ) {
            // TODO
            return true;
        } else if ( attributeName == "storageindex" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "datacatalog" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "datarequirements" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "dataaccessprotocol" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "virtualorganisation" ) {
            if ( !bool( jobTree["JDLDescription"] ) ) jobTree.NewChild("JDLDescription");
            if ( !bool( jobTree["JDLDescription"]["VirtualOrganisation"] ) ) jobTree["JDLDescription"].NewChild("VirtualOrganisation");
            jobTree["JDLDescription"]["VirtualOrganisation"] = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "queuename" ) {
            if ( !bool( jobTree["JDLDescription"] ) ) jobTree.NewChild("JDLDescription");
            if ( !bool( jobTree["JDLDescription"]["QueueName"] ) ) jobTree["JDLDescription"].NewChild("QueueName");
            jobTree["JDLDescription"]["QueueName"] = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "retrycount" ) {
            int count = atoi( simpleJDLvalue( attributeValue ).c_str() );
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Reruns"] ) ) jobTree["JobDescription"].NewChild("Reruns");
            else {
                if ( atoi( ((std::string) jobTree["JobDescription"]["Reruns"]).c_str() ) > count ) count = atoi( ((std::string) jobTree["JobDescription"]["Reruns"]).c_str() );
            }
            std::string new_value;
            std::stringstream ss;
            ss << count;
            ss >> new_value;
            jobTree["JobDescription"]["Reruns"] = new_value;
            return true;
        } else if ( attributeName == "shallowretrycount" ) {
            int count = atoi( simpleJDLvalue( attributeValue ).c_str() );
            if ( !bool( jobTree["JobDescription"] ) ) jobTree.NewChild("JobDescription");
            if ( !bool( jobTree["JobDescription"]["Reruns"] ) ) jobTree["JobDescription"].NewChild("Reruns");
            else {
                if ( atoi( ((std::string) jobTree["JobDescription"]["Reruns"]).c_str() ) > count ) count = atoi( ((std::string) jobTree["JobDescription"]["Reruns"] ).c_str() );
            }
            std::string new_value;
            std::stringstream ss;
            ss << count;
            ss >> new_value;
            jobTree["JobDescription"]["Reruns"] = new_value;
            return true;
        } else if ( attributeName == "lbaddress" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "myproxyserver" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "hlrlocation" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "jobprovenance" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "nodenumber" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "jobsteps" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "currentstep" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "jobstate" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "listenerport" ) {
            // Not supported yet
            return true;    
        } else if ( attributeName == "listenerhost" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "listenerpipename" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "requirements" ) {
            // It's too complicated to determinize the right conditions, because the definition language is
            // LRMS specific.
            return true;
        } else if ( attributeName == "rank" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "fuzzyrank" ) {
            // Not supported yet
            return true;
        } else if ( attributeName == "usertags" ) {
            // They have no standard and no meaning.
            return true;
        }
        if ( DEBUGX ) std::cerr << "[JDL Parser]: Unknown attribute name: '" << attributeName << "', with value: " << attributeValue << std::endl;
        return false;
    }

    bool JDLParser::parse( Arc::XMLNode& jobTree, const std::string source ) {
        unsigned int first = source.find_first_of( "[" );
        unsigned int last = source.find_last_of( "]" );
        if ( first == std::string::npos || last == std::string::npos ) {
            if ( DEBUGX ) std::cerr << "[JDLParser] There is at least one necessary ruler character missing. ('[' or ']')" << std::endl;
            return false;
        }
        std::string input_text = source.substr( first+1, last-first-1 );

        //Remove multiline comments
        unsigned int comment_start = 0;
        while ( (comment_start = input_text.find("/*", comment_start)) != std::string::npos) {
            input_text.erase(input_text.begin() + comment_start, input_text.begin() + input_text.find("*/", comment_start) + 2);
        }

        std::string wcpy = "";
        std::vector<std::string> lines = sm.split(input_text, "\n");
        for ( unsigned int i=0; i < lines.size(); ) {
            // Remove empty lines
            std::string trimmed_line = sm.trim(lines[i]);
            if ( trimmed_line.length() == 0) lines.erase(lines.begin() +i);
            // Remove lines starts with '#' - Comments
            else if ( trimmed_line.length() >= 1 && trimmed_line.substr(0,1) == "#") lines.erase(lines.begin() +i);
            // Remove lines starts with '//' - Comments
            else if ( trimmed_line.length() >= 2 && trimmed_line.substr(0,2) == "//") lines.erase(lines.begin() +i);
            else {
                wcpy += lines[i++] + "\n";
            }
        }
  
        if ( !splitJDL(wcpy, lines) ) {
            if ( DEBUGX ) std::cerr << "[JDLParser] Syntax error found during the split function." << std::endl;
            return false;
        }
        if (lines.size() <= 0) {
            if ( DEBUGX ) std::cerr << "[JDLParser] Lines count is zero or other funny error has occurred." << std::endl;
            return false;
        }
        
        for ( unsigned int i=0; i < lines.size(); i++) {
            unsigned int equal_pos = lines[i].find_first_of("=");
            if ( equal_pos == std::string::npos ) {
                if ( i == lines.size()-1 ) continue;
                else {
                    if ( DEBUGX ) std::cerr << "[JDLParser] JDL syntax error. There is at least one equal sign missing where it would be expected." << std::endl;
                    return false;
                }
            }    
            if (!handleJDLattribute( sm.trim( lines[i].substr(0, equal_pos) ), sm.trim( lines[i].substr(equal_pos+1, std::string::npos) ), jobTree)) return false;
        }
        return true;
    }

    //This feature is not supported yet
    bool JDLParser::getProduct( const Arc::XMLNode& jobTree, std::string& product ) {
        if ( DEBUGX ) std::cerr << "[JDLParser] Converting to JDL - This feature is not supported yet" << std::endl;
        return false;
    }

} // namespace Arc
