#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <time.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/client/JobInnerRepresentation.h>
#include <arc/Logger.h>

#include "JDLParser.h"

namespace Arc {


    bool JDLParser::splitJDL(std::string original_string, std::vector<std::string>& lines) const {

        // Clear the return variable
        lines.clear();
      
        std::string jdl_text = original_string;
      
        bool quotation = false;
        std::vector<char> stack;
        std::string actual_line;
  
        for (int i=0; i<jdl_text.size()-1; i++) {
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

    std::string JDLParser::simpleJDLvalue(std::string attributeValue) const {
        std::string whitespaces (" \t\f\v\n\r");
        unsigned long last_pos = attributeValue.find_last_of( "\"" );
        // If the text is not between quotation marks, then return with the original form
        if (attributeValue.substr( attributeValue.find_first_not_of( whitespaces ), 1 ) != "\"" || last_pos == std::string::npos )
            return sm.trim( attributeValue );
        // Else remove the marks and return with the quotation's content
        else 
            return attributeValue.substr( attributeValue.find_first_of( "\"" )+1,  last_pos - attributeValue.find_first_of( "\"" ) -1 );
    }

    std::vector<std::string> JDLParser::listJDLvalue( std::string attributeValue ) const {
        std::vector<std::string> elements;
        unsigned long first_bracket = attributeValue.find_first_of( "{" );
        if ( first_bracket == std::string::npos ) {
            elements.push_back( simpleJDLvalue( attributeValue ) );
            return elements;
        }
        unsigned long last_bracket = attributeValue.find_last_of( "}" );
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

    bool JDLParser::handleJDLattribute(std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation) const {

        // To do the attributes name case-insensitive do them lowercase and remove the quotiation marks
        attributeName = sm.toLowerCase( attributeName );
        if ( attributeName == "type" ) {
            std::string value = sm.toLowerCase( simpleJDLvalue( attributeValue ) );
            if ( value == "job" ) return true;
            if ( value == "dag" ) {
                logger.msg(DEBUG, "[JDLParser] This kind of JDL decriptor is not supported yet: %s", value);
                return false; // This kind of JDL decriptor is not supported yet
            }
            if ( value == "collection" ) {
                logger.msg(DEBUG, "[JDLParser] This kind of JDL decriptor is not supported yet: %s", value);
                return false; // This kind of JDL decriptor is not supported yet
            }
            logger.msg(DEBUG, "[JDLParser] Attribute name: %s, has unknown value: %s", attributeName , value);
            return false; // Unknown attribute value - error
        } else if ( attributeName == "jobtype") {
            return true; // Skip this attribute
        } else if ( attributeName == "executable" ) {
            innerRepresentation.Executable = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "arguments" ) {
            std::string value = simpleJDLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            for ( std::vector<std::string>::const_iterator it = parts.begin(); it < parts.end(); it++ ) {
                innerRepresentation.Argument.push_back( (*it) );
            }
            return true;
        } else if ( attributeName == "stdinput" ) {
            innerRepresentation.Input =  simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stdoutput" ) {
            innerRepresentation.Output = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stderror" ) {
            innerRepresentation.Error = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "inputsandbox" ) {
            std::vector<std::string> inputfiles = listJDLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=inputfiles.begin(); it<inputfiles.end(); it++) {
                Arc::FileType file;
                std::vector<std::string> parts = sm.split( (*it), "/" );
                file.Name = parts.back();
                Arc::SourceType source;
                source.URI = *it;
                source.Threads = -1;
                file.Source.push_back(source);
                //initializing this variables
                file.KeepData = false;
                file.IsExecutable = false;
                file.DownloadToCache = false;
                innerRepresentation.File.push_back(file);
            }
            return true;
        } else if ( attributeName == "inputsandboxbaseuri" ) {
//            innerRepresentation.JDL_elements["inputsandboxbaseuri"] = simpleJDLvalue( attributeValue ); 

            std::list<Arc::FileType>::iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::SourceType>::iterator i_source;
                for (i_source = (*iter).Source.begin(); i_source != (*iter).Source.end(); i_source++){
                    if ( (*i_source).URI.Host() == "" )
                       (*i_source).URI.ChangeHost(simpleJDLvalue( attributeValue ));
                }
            }
            std::list<Arc::DirectoryType>::iterator iter_d;
            for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
                std::list<Arc::SourceType>::iterator i_source;
                for (i_source = (*iter).Source.begin(); i_source != (*iter).Source.end(); i_source++){
                    if ( (*i_source).URI.Host() == "" )
                       (*i_source).URI.ChangeHost(simpleJDLvalue( attributeValue ));
                }
            }
            return true;
        } else if ( attributeName == "outputsandbox" ) {
            std::vector<std::string> inputfiles = listJDLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=inputfiles.begin(); it<inputfiles.end(); it++) {
                Arc::FileType file;
                file.Name = (*it);
                Arc::TargetType target;
                target.URI = *it;
                target.Threads = -1;
                target.Mandatory = false;
                target.NeededReplicas = -1;
                file.Target.push_back(target);
                //initializing this variables
                file.KeepData = false;
                file.IsExecutable = false;
                file.DownloadToCache = false;
                innerRepresentation.File.push_back(file);
            }
            return true;
        } else if ( attributeName == "outputsandboxdesturi" ) {
            std::vector< std::string > value = listJDLvalue( attributeValue );
            int i=0;
            for (std::list<Arc::FileType>::iterator it=innerRepresentation.File.begin(); 
                                                          it!=innerRepresentation.File.end(); it++) {
                if ( !(*it).Target.empty() ) {
                    if ( i < value.size() ) {
                        (*it).Target.begin()->URI = value[i];
                        i++;
                    }
                    else {
                        logger.msg(DEBUG, "Not enought outputsandboxdesturi element!");
                        return false;
                    }

                }
            }
            return true;
        } else if ( attributeName == "outputsandboxbaseuri" ) {
//            innerRepresentation.JDL_elements["outputsandboxbaseuri"] = simpleJDLvalue( attributeValue ); 

            std::list<Arc::FileType>::iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::iterator i_target;
                for (i_target = (*iter).Target.begin(); i_target != (*iter).Target.end(); i_target++){
                    if ( (*i_target).URI.Host() == "" )
                       (*i_target).URI.ChangeHost(simpleJDLvalue( attributeValue ));
                }
            }
            std::list<Arc::DirectoryType>::iterator iter_d;
            for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
                std::list<Arc::TargetType>::iterator i_target;
                for (i_target = (*iter).Target.begin(); i_target != (*iter).Target.end(); i_target++){
                    if ( (*i_target).URI.Host() == "" )
                       (*i_target).URI.ChangeHost(simpleJDLvalue( attributeValue ));
                }
            }
            return true;
        } else if ( attributeName == "batchsystem" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["BatchSystem"] = "\"" + simpleJDLvalue( attributeValue ) + "\""; 
            return true;
        } else if ( attributeName == "prologue" ) {
            innerRepresentation.Prologue.Name = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "prologuearguments" ) {
            std::string value = simpleJDLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            for ( std::vector<std::string>::const_iterator it = parts.begin(); it < parts.end(); it++ ) {
                innerRepresentation.Prologue.Arguments.push_back( *it );
            }
            return true;
        } else if ( attributeName == "epilogue" ) {
            innerRepresentation.Epilogue.Name = simpleJDLvalue( attributeValue );
            return true;    
        } else if ( attributeName == "epiloguearguments" ) {
            std::string value = simpleJDLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            for ( std::vector<std::string>::const_iterator it = parts.begin(); it < parts.end(); it++ ) {
                innerRepresentation.Epilogue.Arguments.push_back( *it );
            }
            return true;
        } else if ( attributeName == "allowzippedisb" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["AllowZippedISB"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "zippedisb" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ZippedISB"] = "\"" + simpleJDLvalue( attributeValue ) + "\""; 
            return true;
        } else if ( attributeName == "expirytime" ) {
            int lifeTimeSeconds = atoi( simpleJDLvalue( attributeValue ).c_str() );
            int nowSeconds = time(NULL);
            std::stringstream ss;
            ss << lifeTimeSeconds - nowSeconds;
            ss >> attributeValue;
    
            Period time( attributeValue );
            innerRepresentation.SessionLifeTime = time;
            return true;
        } else if ( attributeName == "environment" ) {
            std::vector<std::string> variables = listJDLvalue( attributeValue );
    
            for (std::vector<std::string>::const_iterator it = variables.begin(); it < variables.end(); it++) {
                unsigned long equal_pos = (*it).find_first_of( "=" );
                if ( equal_pos != std::string::npos ) {
                    Arc::EnvironmentType env;
                    env.name_attribute = (*it).substr( 0,equal_pos );
                    env.value = (*it).substr( equal_pos+1, std::string::npos );
                    innerRepresentation.Environment.push_back(env);
                } else {
                    logger.msg(DEBUG, "[JDLParser] Environment variable has been defined without any equal sign.");
                    return false;
                }
            }
            return true;
        } else if ( attributeName == "perusalfileenable" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["PerusalFileEnable"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "perusaltimeinterval" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["PerusalTimeInterval"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "perusalfilesdesturi" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["PerusalFilesDestURI"] = "\"" + simpleJDLvalue( attributeValue ) + "\""; 
            return true;    
        } else if ( attributeName == "inputdata" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "outputdata" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "storageindex" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "datacatalog" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "datarequirements" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["DataRequirements"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "dataaccessprotocol" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["DataAccessProtocol"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "virtualorganisation" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["VirtualOrganisation"] = "\"" + simpleJDLvalue( attributeValue ) + "\""; 
            return true;
        } else if ( attributeName == "queuename" ) {
            innerRepresentation.QueueName = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "retrycount" ) {
            int count = atoi( simpleJDLvalue( attributeValue ).c_str() );
            if ( innerRepresentation.LRMSReRun > count ) count = innerRepresentation.LRMSReRun;
            innerRepresentation.LRMSReRun = count;
            return true;
        } else if ( attributeName == "shallowretrycount" ) {
            int count = atoi( simpleJDLvalue( attributeValue ).c_str() );
            if ( innerRepresentation.LRMSReRun > count ) count = innerRepresentation.LRMSReRun;
            innerRepresentation.LRMSReRun = count;
            return true;
        } else if ( attributeName == "lbaddress" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["LBAddress"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "myproxyserver" ) {
            Arc::URL url(simpleJDLvalue( attributeValue ));
            innerRepresentation.CredentialService = url;
            return true;
        } else if ( attributeName == "hlrlocation" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["HLRLocation"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "jobprovenance" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["JobProvenance"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "nodenumber" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["NodeNumber"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "jobsteps" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "currentstep" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "jobstate" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "listenerport" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ListenerPort"] = simpleJDLvalue( attributeValue ); 
            return true;    
        } else if ( attributeName == "listenerhost" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ListenerHost"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "listenerpipename" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ListenerPipeName"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "requirements" ) {
            // It's too complicated to determinize the right conditions, because the definition language is
            // LRMS specific.
            // Only store it.
            innerRepresentation.JDL_elements["Requirements"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "rank" ) {
            innerRepresentation.Rank = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "fuzzyrank" ) {
            bool fuzzyrank = false;
            if (  Arc::upper(simpleJDLvalue( attributeValue )) == "TRUE") fuzzyrank = true;
            innerRepresentation.FuzzyRank = fuzzyrank;
            return true;
        } else if ( attributeName == "usertags" ) {
            // They have no standard and no meaning.
            innerRepresentation.UserTag.push_back( simpleJDLvalue( attributeValue ) );
            return true;
        } else if ( attributeName == "outputse" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["OutputSE"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "shortdeadlinejob" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ShortDeadlineJob"] = simpleJDLvalue( attributeValue ); 
            return true;
        }
        logger.msg(DEBUG, "[JDL Parser]: Unknown attribute name: \'%s\', with value: %s", attributeName, attributeValue);
        return false;
    }

    bool JDLParser::parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) {
        unsigned long first = source.find_first_of( "[" );
        unsigned long last = source.find_last_of( "]" );
        if ( first == std::string::npos || last == std::string::npos ) {
        logger.msg(DEBUG, "[JDLParser] There is at least one necessary ruler character missing. ('[' or ']')");
            return false;
        }
        std::string input_text = source.substr( first+1, last-first-1 );

        //Remove multiline comments
        unsigned long comment_start = 0;
        while ( (comment_start = input_text.find("/*", comment_start)) != std::string::npos) {
            input_text.erase(input_text.begin() + comment_start, input_text.begin() + input_text.find("*/", comment_start) + 2);
        }

        std::string wcpy = "";
        std::vector<std::string> lines = sm.split(input_text, "\n");
        for ( unsigned long i=0; i < lines.size(); ) {
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
            logger.msg(DEBUG, "[JDLParser] Syntax error found during the split function.");
            return false;
        }
        if (lines.size() <= 0) {
            logger.msg(DEBUG, "[JDLParser] Lines count is zero or other funny error has occurred.");
            return false;
        }
        
        for ( unsigned long i=0; i < lines.size(); i++) {
            unsigned long equal_pos = lines[i].find_first_of("=");
            if ( equal_pos == std::string::npos ) {
                if ( i == lines.size()-1 ) continue;
                else {
                    logger.msg(DEBUG, "[JDLParser] JDL syntax error. There is at least one equal sign missing where it would be expected.");
                    return false;
                }
            }    
            if (!handleJDLattribute( sm.trim( lines[i].substr(0, equal_pos) ), sm.trim( lines[i].substr(equal_pos+1, std::string::npos) ), innerRepresentation)) return false;
        }
        return true;
    }

    bool JDLParser::getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) const {
        product = "[\n  Type = \"job\";\n";
        if (!innerRepresentation.Executable.empty()) {
            product += "  Executable = \"";
            product += innerRepresentation.Executable;
            product += "\";\n";
        }
        if (!innerRepresentation.Argument.empty()) {
            product += "  Arguments = \"";
            bool first = true;
            for (std::list<std::string>::const_iterator it=innerRepresentation.Argument.begin();
                            it!=innerRepresentation.Argument.end(); it++) {
                 if ( !first ) product += " ";
                else first = false;
                product += *it;
            }
            product += "\";\n";
        }
        if (!innerRepresentation.Input.empty()) {
            product += "  StdInput = \"";
            product += innerRepresentation.Input;
            product += "\";\n";
        }
        if (!innerRepresentation.Output.empty()) {
            product += "  StdOutput = \"";
            product += innerRepresentation.Output;
            product += "\";\n";
        }
        if (!innerRepresentation.Error.empty()) {
            product += "  StdError = \"";
            product += innerRepresentation.Error;
            product += "\";\n";
        }
        if (!innerRepresentation.Environment.empty()) {
            product += "  Environment = {\n";
            for (std::list<Arc::EnvironmentType>::const_iterator it = innerRepresentation.Environment.begin(); 
                 it != innerRepresentation.Environment.end(); it++) {
                product += "    \"" + (*it).name_attribute + "=" + (*it).value + "\"";
                if (it++ != innerRepresentation.Environment.end()) { 
                  product += ",";
                  it--;
                }
                product += "\n";
            }
            product += "    };\n";
        }
        if (!innerRepresentation.Prologue.Name.empty()) {
            product += "  Prologue = \"";
            product += innerRepresentation.Prologue.Name.empty();
            product += "\";\n";
        }
        if (!innerRepresentation.Prologue.Arguments.empty()) {
            product += "  PrologueArguments = \"";
            bool first = true;
            for (std::list<std::string>::const_iterator iter = innerRepresentation.Prologue.Arguments.begin();
                               iter != innerRepresentation.Prologue.Arguments.end(); iter++){
                if ( !first ) product += " ";
                else first = false;
                product += *iter;
            }
            product += "\";\n";
        }
        if (!innerRepresentation.Epilogue.Name.empty()) {
            product += "  Epilogue = \"";
            product += innerRepresentation.Epilogue.Name;
            product += "\";\n";
        }
        if (!innerRepresentation.Epilogue.Arguments.empty()) {
            product += "  EpilogueArguments = \"";
            bool first = true;
            for (std::list<std::string>::const_iterator iter = innerRepresentation.Epilogue.Arguments.begin();
                               iter != innerRepresentation.Epilogue.Arguments.end(); iter++){
                if ( !first ) product += " ";
                else first = false;
                product += *iter;
            }
            product += "\";\n";
        }
        if (!innerRepresentation.File.empty() ||
            !innerRepresentation.Executable.empty() ||
            !innerRepresentation.Input.empty()) {

            bool executable = false;
            bool input = false;
            bool empty = true;
            product += "  InputSandbox = {\n";
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                if ( (*iter).Name == innerRepresentation.Executable) executable = true;
                if ( (*iter).Name == innerRepresentation.Input) input = true;
                std::list<Arc::SourceType>::const_iterator it_source;
                for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                    if (it_source == (*iter).Source.begin() && empty) {
                        product += "    \"" +  (*it_source).URI.fullstr() + "\"";
                    }
                    else {
                        product += ",\n    \"" +  (*it_source).URI.fullstr() + "\"";
                    }
                    empty = false;
                }
            }
            if (!innerRepresentation.Executable.empty() && !executable) {
                if (!empty) product += ",\n";
                empty = false;
                product += "    \"" + innerRepresentation.Executable + "\"";
            }
            if (!innerRepresentation.Input.empty() && !input) {
                if (!empty) product += ",\n";
                empty = false;
                product += "    \"" + innerRepresentation.Input + "\"";
            }
            product += "\n    };\n";
         }
        if (!innerRepresentation.File.empty() ||
            !innerRepresentation.Error.empty() ||
            !innerRepresentation.Output.empty()) {

            std::list<Arc::FileType>::const_iterator iter;
            bool first = true;
            bool error = false;
            bool output = false;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                if ( (*iter).Name == innerRepresentation.Error) error = true;
                if ( (*iter).Name == innerRepresentation.Output) output = true;
                if ( !(*iter).Target.empty()) {
                   if (!first) {
                      product += ",";
                   }
                   else {
                      product += "  OutputSandbox = {";
                      first = false;
                   }
                   product += " \"" +  (*iter).Name + "\"";
                }
            }
            if (!innerRepresentation.Error.empty() && !error) {
                if ( first ) {
                    product += "  OutputSandbox = {";
                    first = false;
                }
                else {
                    product += ", ";
                }
                product += " \"" + innerRepresentation.Error + "\"";
            }
            if (!innerRepresentation.Output.empty() && !output) {
                if ( first ) {
                    product += "  OutputSandbox = {";
                    first = false;
                }
                else {
                    product += ", ";
                }
                product += " \"" + innerRepresentation.Output + "\"";
            }
            if ( !first )
               product += " };\n";
        }
        if (!innerRepresentation.File.empty() ||
            !innerRepresentation.Error.empty() ||
            !innerRepresentation.Output.empty()) {

            std::list<Arc::FileType>::const_iterator iter;
            bool first = true;
            bool error = false;
            bool output = false;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                if ( (*iter).Name == innerRepresentation.Error) error = true;
                if ( (*iter).Name == innerRepresentation.Output) output = true;
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                    if ( first ){
                       product += "  OutputSandboxDestURI = {\n";
                      // first = false;
                    }
                    if (it_target == (*iter).Target.begin() && first) {
                       first = false;
                       product += "    \"" +  (*it_target).URI.fullstr() + "\"";
                    }
                    else {
                        product += ",\n    \"" +  (*it_target).URI.fullstr() + "\"";
                    }
//                    product += "    \"" +  (*it_target).URI.fullstr() + "\"";
  //                  if (it_target++ != (*iter).Target.end()) {
    //                   product += ",";
      //                 it_target--;
        //            }
          //          product += "\n";
                }
            }
            if (!innerRepresentation.Error.empty() && !error) {
                if ( first ) {
                    product += "  OutputSandboxDestURI = {";
                    first = false;
                }
                else {
                    product += ", \n";
                }
                Arc::URL errorURL("gsiftp://localhost/" + innerRepresentation.Error);
                product += "    \"" + errorURL.fullstr() + "\"";
            }
            if (!innerRepresentation.Output.empty() && !output) {
                if ( first ) {
                    product += "  OutputSandboxDestURI = {";
                    first = false;
                }
                else {
                    product += ", \n";
                }
                Arc::URL outputURL("gsiftp://localhost/" + innerRepresentation.Output);
                product += "    \"" + outputURL.fullstr() + "\"";
            }
            if ( !first)
               product += "\n    };\n";
        }
        if (!innerRepresentation.QueueName.empty()) {
            product += "  QueueName = \"";
            product +=  innerRepresentation.QueueName;
            product += "\";\n";
        }
        if (innerRepresentation.LRMSReRun > -1) {
            product += "  RetryCount = \"";
            std::ostringstream oss;
            oss << innerRepresentation.LRMSReRun;
            product += oss.str();
            product += "\";\n";
        }
        if (innerRepresentation.SessionLifeTime != -1) {
            std::string outputValue;
            int attValue = atoi( ((std::string) innerRepresentation.SessionLifeTime).c_str() );
            int nowSeconds = time(NULL);
            std::stringstream ss;
            ss << attValue + nowSeconds;
            ss >> outputValue;
            product += "  ExpiryTime = \"";
            product += outputValue;
            product += "\";\n";
        }
        if (bool(innerRepresentation.CredentialService)) {
            product += "  MyProxyServer = \"";
            product +=  innerRepresentation.CredentialService.fullstr();
            product += "\";\n";
        }
        if (!innerRepresentation.Rank.empty()) {
            product += "  Rank = \"";
            product +=  innerRepresentation.Rank;
            product += "\";\n";
        }
        if (innerRepresentation.FuzzyRank) {
            product += "  FuzzyRank = true;\n";
        }
        if (!innerRepresentation.UserTag.empty()) {
            product += "  UserTag = ";
            bool first = true;
            std::list<std::string>::const_iterator iter;
            for (iter = innerRepresentation.UserTag.begin(); iter != innerRepresentation.UserTag.end(); iter++){
                if (!first) {
                   product += ",";
                }
                else {
                   first = false;
                }
                product +=  *iter;
            }
            product += ";\n";
        }
        if ( !innerRepresentation.JDL_elements.empty() ) {
          std::map<std::string, std::string>::const_iterator it;
          for ( it = innerRepresentation.JDL_elements.begin(); it != innerRepresentation.JDL_elements.end(); it++ ) {
              product += "  ";
              product += it->first;
              product += " = ";
              product += it->second;
              product += ";\n";
          }
        }
        product += "]";

        return true;
    }

} // namespace Arc
