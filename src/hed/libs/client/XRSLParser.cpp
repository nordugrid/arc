#include <string>
#include <vector>
#include <list>
#include <map>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/job/runtimeenvironment.h>
#include <arc/data/CheckSum.h>
#include <arc/client/JobInnerRepresentation.h>
#include <arc/Logger.h>

#include "XRSLParser.h"

namespace Arc {


    std::string XRSLParser::simpleXRSLvalue(std::string attributeValue) const {
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

    std::vector<std::string> XRSLParser::listXRSLvalue(std::string attributeValue) const {

        std::string whitespaces (" \t\f\v\n\r");

        std::vector<std::string> attributes;
        unsigned long start_pointer;
        for ( unsigned long end_pointer = start_pointer = attributeValue.find_first_not_of(whitespaces); start_pointer != std::string::npos; ) {
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

    std::vector< std::vector<std::string> > XRSLParser::doubleListXRSLvalue(std::string attributeValue ) const {

        std::vector< std::vector<std::string> > new_attributeValue;
        std::string whitespaces (" \t\f\v\n\r");

        unsigned long outer_start_pointer;
        unsigned long outer_end_pointer = 0;
        for ( outer_start_pointer = attributeValue.find_first_of("(", outer_end_pointer); outer_start_pointer != std::string::npos; outer_start_pointer = attributeValue.find_first_of("(", outer_end_pointer) ) {
            unsigned long old_outer_end_pointer = outer_end_pointer;
            outer_end_pointer = attributeValue.find_first_of(")", outer_start_pointer);
            if ( attributeValue.find_first_of("(", old_outer_end_pointer+1) < outer_end_pointer &&
                 attributeValue.find_first_of(")", outer_end_pointer+1) != std::string::npos ) {
               outer_end_pointer = attributeValue.find_first_of(")", outer_end_pointer+1);
            }

            std::string inner_attributeValue = attributeValue.substr(outer_start_pointer+1,outer_end_pointer-outer_start_pointer-1);
            std::vector<std::string> attributes;
            unsigned long start_pointer;
            for ( unsigned long end_pointer = start_pointer = inner_attributeValue.find_first_not_of(whitespaces); start_pointer != std::string::npos; ) {
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

    bool XRSLParser::handleXRSLattribute(std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation) {

        // To do the attributes name case-insensitive do them lowercase and remove the quotiation marks
        attributeName = simpleXRSLvalue( sm.toLowerCase( attributeName ) );  

        if ( attributeName == "executable" ) {
            innerRepresentation.Executable = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "arguments") {
            std::vector<std::string> attributes = listXRSLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=attributes.begin(); it<attributes.end(); it++) {
                innerRepresentation.Argument.push_back( (*it) );
            }
            return true;
        } else if ( attributeName == "stdin" ) {
            innerRepresentation.Input =  simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stdout" ) {
            innerRepresentation.Output = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stderr" ) {
            innerRepresentation.Error = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "inputfiles" ) {
            input_files = attributeValue;
            return true;
        } else if ( attributeName == "executables" ) {
            std::vector<std::string> attributes = listXRSLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=attributes.begin(); it<attributes.end(); it++) {
                for (std::list<Arc::FileType>::iterator it2 = innerRepresentation.File.begin(); 
                                                        it2 != innerRepresentation.File.end(); it2++) {
                    if ( (*it2).Name == (*it) ) (*it2).IsExecutable = true;
                }
            }
            return true;
        } else if ( attributeName == "cache" ) {
            if ( Arc::upper(simpleXRSLvalue( attributeValue )) == "YES"){
               innerRepresentation.cached = true;
               for (std::list<Arc::FileType>::iterator it = innerRepresentation.File.begin(); 
                                                       it != innerRepresentation.File.end(); it++) {
                   if ( !(*it).Source.empty() ) (*it).DownloadToCache = true;
               }
            }
            return true;
        } else if ( attributeName == "outputfiles" ) {
            output_files = attributeValue;
            return true;
        } else if ( attributeName == "queue" ) {
            innerRepresentation.QueueName = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "starttime" ) {
            Time time(simpleXRSLvalue( attributeValue ) );
            innerRepresentation.ProcessingStartTime = time;
            return true;
        } else if ( attributeName == "lifetime" ) {
            Period time(simpleXRSLvalue( attributeValue ) );
            innerRepresentation.SessionLifeTime = time;
            return true;
        } else if ( attributeName == "cputime" ) {
            Period time(simpleXRSLvalue( attributeValue ), Arc::PeriodMinutes );
            innerRepresentation.TotalCPUTime = time;
            return true;
        } else if ( attributeName == "walltime" ) {
            Period time(simpleXRSLvalue( attributeValue ), Arc::PeriodMinutes);
            innerRepresentation.TotalWallTime = time;
            return true;
        } else if ( attributeName == "gridtime" ) {
            innerRepresentation.ReferenceTime.value = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "benchmarks" ) {
            //for example: (benchmarks=("mybenchmark" "10" "1 hour, 30 minutes"))
            std::vector<std::string> attributes = listXRSLvalue( attributeValue );
            if ( (int)attributes.size() > 2 ) { 
               innerRepresentation.ReferenceTime.benchmark_attribute = attributes[0];
               innerRepresentation.ReferenceTime.value_attribute = attributes[1];
               innerRepresentation.ReferenceTime.value = attributes[2];
            }
            return true;
        } else if ( attributeName == "memory" ) {
            innerRepresentation.IndividualPhysicalMemory = stringtoi(simpleXRSLvalue( attributeValue ));
            return true;
        } else if ( attributeName == "disk" ) {
            innerRepresentation.DiskSpace = stringtoi(simpleXRSLvalue( attributeValue ));
            return true;
        } else if ( attributeName == "runtimeenvironment" ) {
            Arc::RunTimeEnvironmentType runtime;
            RuntimeEnvironment rt(simpleXRSLvalue( attributeValue ));
            runtime.Name =  rt.Name();
            if (!rt.Version().empty())
               runtime.Version.push_back(rt.Version());
            innerRepresentation.RunTimeEnvironment.push_back( runtime );
            return true;
        } else if ( attributeName == "middleware" ) {
            innerRepresentation.CEType = simpleXRSLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "opsys" ) {
            innerRepresentation.OSName = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "join" ) {
            if ( Arc::upper(simpleXRSLvalue( attributeValue )) == "TRUE")
               innerRepresentation.Join = true;
            return true;
        } else if ( attributeName == "gmlog" ) {
            innerRepresentation.LogDir = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "jobname" ) {
            innerRepresentation.JobName = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "ftpthreads" ) {
            int threads = stringtoi( simpleXRSLvalue( attributeValue ) );
            //set the File
            std::list<Arc::FileType>::iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
              std::list<Arc::SourceType>::iterator it_source;
              for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                  if ( (*it_source).Threads > threads || (*it_source).Threads == -1 )
                     (*it_source).Threads = threads;
              }
              std::list<Arc::TargetType>::iterator it_target;
              for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                  if ( (*it_target).Threads > threads || (*it_target).Threads == -1 )
                     (*it_target).Threads = threads;
              }
            }
            //set the Directory
            std::list<Arc::DirectoryType>::iterator iterd;
            for (iterd = innerRepresentation.Directory.begin(); iterd != innerRepresentation.Directory.end(); iterd++){
              std::list<Arc::SourceType>::iterator it_source;
              for (it_source = (*iterd).Source.begin(); it_source != (*iterd).Source.end(); it_source++){
                  if ( (*it_source).Threads > threads || (*it_source).Threads == -1 )
                     (*it_source).Threads = threads;
              }
              std::list<Arc::TargetType>::iterator it_target;
              for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                  if ( (*it_target).Threads > threads || (*it_target).Threads == -1 )
                     (*it_target).Threads = threads;
              }
            }
            return true;
        } else if ( attributeName == "acl" ) {
            Arc::XMLNode node(simpleXRSLvalue( attributeValue ));
            node.New(innerRepresentation.AccessControl);
            return true;
        } else if ( attributeName == "cluster" ) {
            URL url(simpleXRSLvalue( attributeValue ));
            innerRepresentation.EndPointURL = url; 
            return true;
        } else if ( attributeName == "notify" ) {
            std::string value = simpleXRSLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            Arc::NotificationType nofity;
            // When the state or the e-mail address missed.
            if ( parts.size() < 2 ){ 
                logger.msg(DEBUG, "Invalid notify attribute: %s", attributeValue);
                return false;
            }

            for (std::vector< std::string >::const_iterator it=parts.begin(); it<parts.end(); it++) {
              //flags or e-mail address
              if ( it == parts.begin() ){
                 //flag conversion to Job State
                 for (int i=0; i!= (*it).length(); i++) {
                     switch ( (*it)[i] ){ 
                         case 'b': nofity.State.push_back( "PREPARING" ); break;
                         case 'q': nofity.State.push_back( "INLRMS" ); break;
                         case 'f': nofity.State.push_back( "FINISHING" ); break;
                         case 'e': nofity.State.push_back( "FINISHED" ); break;
                         case 'c': nofity.State.push_back( "CANCELLED" ); break;
                         case 'd': nofity.State.push_back( "DELETED" ); break;
                         default:
                            logger.msg(DEBUG, "Invalid notify attribute: %s", (*it)[i]);
                            return false;
                     }
                 }
              }
              else {
                 nofity.Address.push_back( *it );
              }
            }
            innerRepresentation.Notification.push_back( nofity );
            return true;
        } else if ( attributeName == "replicacollection" ) {
            if ( simpleXRSLvalue( attributeValue ) != "" ){
               Arc::URL url(simpleXRSLvalue( attributeValue ));
               std::list<Arc::FileType>::iterator iter;
               for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                    (*iter).DataIndexingService = url;
               }
               std::list<Arc::DirectoryType>::iterator iter_d;
               for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
                    (*iter_d).DataIndexingService = url;
               }
            }
            return true;
        } else if ( attributeName == "rerun" ) {
            innerRepresentation.LRMSReRun = stringtoi(simpleXRSLvalue( attributeValue ));
            return true;
        } else if ( attributeName == "architecture" ) {
            innerRepresentation.Platform = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "nodeaccess" ) {
            if ( simpleXRSLvalue( attributeValue ) == "inbound") {
               innerRepresentation.InBound = true;
            } else if ( simpleXRSLvalue( attributeValue ) == "outbound") {
               innerRepresentation.OutBound = true;
            } else {
                logger.msg(DEBUG, "Invalid NodeAccess value: %s", simpleXRSLvalue( attributeValue ));
                return false;
            }
            return true;
        } else if ( attributeName == "dryrun" ) {
            // Not supported yet, only store it
            innerRepresentation.XRSL_elements["dryrun"] = simpleXRSLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "rsl_substitution" ) {
            std::vector< std::vector<std::string> > attributeVector = doubleListXRSLvalue( attributeValue );
            rsl_substitutions["$(" +(*attributeVector.begin())[0] + ")"] = (*attributeVector.begin())[1];
            return true;
        } else if ( attributeName == "environment" ) {
            std::vector< std::vector<std::string> > attributesDoubleVector = doubleListXRSLvalue( attributeValue );
            for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin();
                                       it<attributesDoubleVector.end(); it++) {
                Arc::EnvironmentType env;
                env.name_attribute = (*it)[1];
                env.value = (*it)[0];
                innerRepresentation.Environment.push_back(env);
           }
            return true;
        } else if ( attributeName == "count" ) {
            innerRepresentation.ProcessPerHost = stringtoi(simpleXRSLvalue( attributeValue ));
            return true;
        } else if ( attributeName == "jobreport" ) {
            Arc::URL url(simpleXRSLvalue( attributeValue ));
            innerRepresentation.RemoteLogging = url;
            return true;
        } else if ( attributeName == "credentialserver" ) {
            Arc::URL url(simpleXRSLvalue( attributeValue ));
            innerRepresentation.CredentialService = url;
            return true;
        }
        logger.msg(DEBUG, "[XRSLParser] Unknown XRSL attribute: %s", attributeName);
        return false;
    }

    bool XRSLParser::parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) {
        //First step: trim the text
        //Second step: distinguis the different attributes & convert string to C char array
        int depth = 0;
        std::string xrsl_text = source.substr(source.find_first_of("&")+1);
        bool comment = false;
        std::string actual_argument = "";
  
        for (unsigned long pos=0; pos < xrsl_text.size(); pos++) {
            char next_char = xrsl_text[pos];
            if ( comment && next_char == ')' && pos >= 1 && xrsl_text[pos-1] == '*' ) comment = false;
            else if ( !comment ) {
                if ( next_char == '*' && pos >= 1 && xrsl_text[pos-1] == '(' ) {
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
                        unsigned long eqpos = wattr.find_first_of("=");
                        if ( eqpos == std::string::npos ) {
                            logger.msg(DEBUG, "[XRSLParser] XRSL Syntax error (attribute declaration without equal sign)");
                            return false;
                        }
                        if ( !handleXRSLattribute( sm.trim( wattr.substr(0,eqpos) ) , sm.trim( wattr.substr(eqpos+1) ) , innerRepresentation) ) return false;
                    } else actual_argument += next_char;
                } else {
                    actual_argument += next_char;
                    if ( pos == xrsl_text.size()-1 ) return false;
                }
            }
        }

        if (depth != 0 ) {
            logger.msg(DEBUG, "[XRSLParser] XRSL Syntax error (bracket mistake)");
            return false;
        }

        std::vector< std::vector<std::string> > attributesDoubleVector = doubleListXRSLvalue( input_files );
        for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin(); it<attributesDoubleVector.end(); it++) {
            Arc::FileType file;
            file.Name = (*it)[0];
            std::string uri;
            Arc::SourceType source;
            if ( (*it)[1] != "" )
                uri = (*it)[1];
            else
                uri = (*it)[0];
            for (std::map<std::string, std::string>::const_iterator iter = rsl_substitutions.begin(); iter != rsl_substitutions.end(); iter++) {
                int pos = uri.find((*iter).first);
                if ( pos!= std::string::npos ) {
                    uri.replace(pos, (*iter).first.length(), (*iter).second);
                    iter = rsl_substitutions.begin();
                }
            }
            source.URI = uri;
            source.Threads = -1;
            file.Source.push_back(source);
            //initializing this variables
            file.KeepData = false;
            file.IsExecutable = false;
            file.DownloadToCache = innerRepresentation.cached;
            innerRepresentation.File.push_back(file);
        }

        attributesDoubleVector = doubleListXRSLvalue( output_files );
        for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin(); it<attributesDoubleVector.end(); it++) {
            Arc::FileType file;
            file.Name = (*it)[0];
            std::string uri;
            Arc::TargetType target;
            if ( (*it)[1] != "" )
                uri = (*it)[1];
            else
                uri = (*it)[0];
            for (std::map<std::string, std::string>::const_iterator iter = rsl_substitutions.begin(); iter != rsl_substitutions.end(); iter++) {
                int pos = uri.find((*iter).first);
                if ( pos!= std::string::npos ) {
                    uri.replace(pos, (*iter).first.length(), (*iter).second);
                    iter = rsl_substitutions.begin();
                }
            }
            target.URI = uri;
            target.Threads = -1;
            file.Target.push_back(target);
            //initializing this variables
            file.KeepData = false;
            file.IsExecutable = false;
            file.DownloadToCache = innerRepresentation.cached;
            innerRepresentation.File.push_back(file);
        }
        return true;
    }

    bool XRSLParser::getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) const {
        product = "&\n";
        if (!innerRepresentation.Executable.empty()) {
            product += "( executable = ";//TODO:
            product += innerRepresentation.Executable;
            product += " )\n";
        }
        if (!innerRepresentation.Argument.empty()) {
            bool first = true;
            for (std::list<std::string>::const_iterator it=innerRepresentation.Argument.begin();
                            it!=innerRepresentation.Argument.end(); it++) {
                if (first){      
                   product += "( arguments = ";
                   if (!innerRepresentation.Executable.empty()) {
                       product += innerRepresentation.Executable;
                       product += " ";
                   }
                   first = false;
                }
                product += *it;
                product += " ";
            }
            if (!first){      
                product += " )\n";
            }
        }
        if (!innerRepresentation.Input.empty()) {
            product += "( stdin = ";
            product += innerRepresentation.Input;
            product += " )\n";
        }
        if (!innerRepresentation.Output.empty()) {
            product += "( stdout = ";
            product += innerRepresentation.Output;
            product += " )\n";
        }
        if (!innerRepresentation.Error.empty()) {
            product += "( stderr = ";
            product += innerRepresentation.Error;
            product += " )\n";
        }
        if (innerRepresentation.TotalCPUTime != -1) {
            std::string outputValue;
            std::stringstream ss;
            ss << innerRepresentation.TotalCPUTime.GetPeriod();
            ss >> outputValue;
            product += "( cputime = ";
            product += outputValue;
            product += " )\n";
        }
        if (innerRepresentation.TotalWallTime != -1) {
            std::string outputValue;
            std::stringstream ss;
            ss << innerRepresentation.TotalWallTime.GetPeriod();
            ss >> outputValue;
            product += "( walltime = ";
            product += outputValue;
            product += " )\n";
        }
        if (innerRepresentation.IndividualPhysicalMemory != -1) {
            std::string outputValue;
            std::stringstream ss;
            ss << innerRepresentation.IndividualPhysicalMemory;
            ss >> outputValue;
            product += "( memory = ";
            product += outputValue;
            product += " )\n";
        }
        if (!innerRepresentation.Environment.empty()) {
            for (std::list<Arc::EnvironmentType>::const_iterator it = innerRepresentation.Environment.begin(); 
                 it != innerRepresentation.Environment.end(); it++) {
               product += "( environment = (";
               product += (*it).value + " " + (*it).name_attribute;
               product += ") )\n";
            }
        }
        if (!innerRepresentation.File.empty()) {
            bool first_time = true;
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::SourceType>::const_iterator it_source;
                for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                    if (first_time){
                       product += "( inputfiles =";
                       first_time = false;
                    }
                    product += " ( \"" + (*iter).Name;

                    // file size added
                    struct stat fileStat;
                    if ( stat((*it_source).URI.Path().c_str(), &fileStat) == 0){
                       std::string filesize;
                       std::stringstream ss;
                       ss << fileStat.st_size;
                       ss >> filesize;
                       product += "\" \"" + filesize + "\"";
                    }
                    else {
                        product += "\" \"" +  (*it_source).URI.fullstr() + "\"";
                    }

                    // checksum added
                    // It is optional!!!
                    int h=open((*it_source).URI.fullstr().c_str(),O_RDONLY);
                    if (h > -1) { /* if we can read that file job can checksum compute */
                       Arc::CRC32Sum crc;
                       char buffer[1024];
                       ssize_t l;
                       size_t ll = 0;
                       for(;;) {
                          if ((l=read(h,buffer,1024)) == -1) {
                            logger.msg(DEBUG, "Error reading file: %s", (*it_source).URI.fullstr());
                            logger.msg(DEBUG, "Could not read file to compute checksum.");
                          }
                          if(l==0) break; ll+=l;
                          crc.add(buffer,l);
                       }
                       close(h);
                       crc.end();
                       /* //TODO: it is not working now!
                       unsigned char *buf;
                       unsigned int len;
                       crc.result(buf,len);
                       std::string file_checksum;
                       std::stringstream ss;
                       ss << buf;
                       ss >> file_checksum;
                       if (!file_checksum.empty())
                          product += " \"" + file_checksum + "\"";
                       */
                    }
                    product +=  " )";
                }
            }
            if (!first_time)
               product += " )\n";
        }
        if (!innerRepresentation.File.empty()) {
            bool first_time = true;
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                if ( (*iter).IsExecutable ) {
                   if ( first_time ) {
                      product += "( executables =";
                      first_time = false;
                   }
                   product += " \"" + (*iter).Name + "\"";
                }
            }
            if ( !first_time ) 
               product += " )\n";
        }
        if (!innerRepresentation.File.empty()) {
            bool first_time = true;
            bool output(false),error(false);
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                    if ( first_time ) {
                       product += "( outputfiles =";
                       first_time = false;
                    }
                    product += " ( \"" + (*iter).Name;
                    if ( (*iter).Name == innerRepresentation.Output ) output = true;
                    if ( (*iter).Name == innerRepresentation.Error ) error = true;
                    if ((*it_target).URI.fullstr().empty()  || (*it_target).URI.Protocol() == "file")
                       product += "\" \"\" )";
                    else 
                       product += "\" " +  (*it_target).URI.fullstr() + " )";
                }
            }
            if ( !first_time ){
               if (!innerRepresentation.Output.empty() && !output)
                  product += " ( \"" + innerRepresentation.Output + "\" \"\" )";
               if (!innerRepresentation.Error.empty() && !error)
                  product += " ( \"" + innerRepresentation.Error + "\" \"\" )";
               product += " )\n";
            }
            else {
               if (!innerRepresentation.Output.empty() ||
                   !innerRepresentation.Error.empty()) product += "( outputfiles =";
               
               if (!innerRepresentation.Output.empty()) product += " ( \"" + innerRepresentation.Output + "\" \"\" )";
               if (!innerRepresentation.Error.empty())  product += " ( \"" + innerRepresentation.Error + "\" \"\" )";
               if (!innerRepresentation.Output.empty() ||
                   !innerRepresentation.Error.empty()) product += " )\n";

            }
        }
        if (!innerRepresentation.QueueName.empty()) {
            product += "( queue = ";
            product +=  innerRepresentation.QueueName;
            product += " )\n";
        }
        if (innerRepresentation.LRMSReRun > -1) {
            product += "( rerun = ";
            std::ostringstream oss;
            oss << innerRepresentation.LRMSReRun;
            product += oss.str();
            product += " )\n";
        }
        if (innerRepresentation.SessionLifeTime != -1) {
            std::string outputValue;
            std::stringstream ss;
            ss << innerRepresentation.SessionLifeTime.GetPeriod();
            ss >> outputValue;
            product += "( lifetime = ";
            product += outputValue;
            product += " )\n";
        }
        // disk is not support on the GM side.
        // in question attribute
        /*if (innerRepresentation.DiskSpace > -1) {
            product += "( disk = ";
            std::ostringstream oss;
            oss << innerRepresentation.DiskSpace;
            product += oss.str();
            product += " )\n";
        }*/
        if (!innerRepresentation.RunTimeEnvironment.empty()) {
            std::list<Arc::RunTimeEnvironmentType>::const_iterator iter;
            for (iter = innerRepresentation.RunTimeEnvironment.begin(); \
                 iter != innerRepresentation.RunTimeEnvironment.end(); iter++){
                product += "( runtimeenvironment = ";
                product +=  (*iter).Name;
                if ( (*iter).Version.begin() != (*iter).Version.end() ){
                   product +=  "-" + *(*iter).Version.begin();
                }
                product += " )\n";
            }
        }
        if (!innerRepresentation.OSName.empty()) {
            product += "( opsys = ";
            product +=  innerRepresentation.OSName;
            product += " )\n";
        }
        // architecture is not support on the GM side.
        // in question attribute
        /*if (!innerRepresentation.Platform.empty()) {
            product += "( architecture = ";
            product +=  innerRepresentation.Platform;
            product += " )\n";
        }*/
        if (innerRepresentation.ProcessPerHost > -1) {
            product += "( count = ";
            std::ostringstream oss;
            oss << innerRepresentation.ProcessPerHost;
            product += oss.str();
            product += " )\n";
        }
        if (innerRepresentation.ProcessingStartTime != -1) {
            product += "( starttime = ";
            product += innerRepresentation.ProcessingStartTime.str(Arc::MDSTime);
            product += " )\n";
        }
        // join is not support on the GM side.
        // in question attribute
        /*if (innerRepresentation.Join) {
            product += "( join = true )\n";
        }*/
        if (!innerRepresentation.LogDir.empty()) {
            product += "( gmlog = ";
            product +=  innerRepresentation.LogDir;
            product += " )\n";
        }
        if (!innerRepresentation.JobName.empty()) {
            product += "( jobname = \"";
            product +=  innerRepresentation.JobName;
            product += "\" )\n";
        }
        if (bool(innerRepresentation.AccessControl)) {
            product += "( acl = ";
            std::string acl;
            innerRepresentation.AccessControl.GetXML(acl, true);
            product +=  acl;
            product += " )\n";
        }
        if (!innerRepresentation.Notification.empty()) {
            for (std::list<Arc::NotificationType>::const_iterator it=innerRepresentation.Notification.begin();
                                     it!=innerRepresentation.Notification.end(); it++) {
                product += "( notify = ";
                for (std::list<std::string>::const_iterator iter=(*it).State.begin();
                            iter!=(*it).State.end(); iter++) {
                    if ( *iter == "PREPARING" ) { product +=  "b"; }
                    else if ( *iter == "INLRMS" ) { product +=  "q"; }
                    else if ( *iter == "FINISHING" ) { product +=  "f"; }
                    else if ( *iter == "FINISHED" ) { product +=  "e"; }
                    else if ( *iter == "CANCELLED" ) { product +=  "c"; }
                    else if ( *iter == "DELETED" ) { product +=  "d"; }
                    else { 
                        logger.msg(DEBUG, "Invalid State [\"%s\"] in the Notification!", *iter );
                    }
                }
                for (std::list<std::string>::const_iterator iter=(*it).Address.begin();
                            iter!=(*it).Address.end(); iter++) {
                     product +=  " " + (*iter);
                }
                product += " )\n";
            }
        }
        if (bool(innerRepresentation.RemoteLogging)) {
            product += "( jobreport = ";
            product +=  innerRepresentation.RemoteLogging.fullstr();
            product += " )\n";
        }
        if (bool(innerRepresentation.CredentialService)) {
            product += "( credentialserver = ";
            product +=  innerRepresentation.CredentialService.fullstr();
            product += " )\n";
        }
        if (!innerRepresentation.File.empty()) {
            bool _true = false;
            bool _false = false;
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::const_iterator it_target;
                if ( (*iter).DownloadToCache ){
                    _true = true;
                }
                else {
                    _false = true;
                }
            }
            if ( _true && !_false) {
                product += "( cahce = \"true\" )\n";
            }
        }
        // benchmarks is not support on the GM side.
        // in question attribute
        /*if (!innerRepresentation.ReferenceTime.value.empty()) {
            product += "( benchmarks = ( \"";
            if ( !innerRepresentation.ReferenceTime.benchmark_attribute.empty() && 
                 !innerRepresentation.ReferenceTime.value_attribute.empty()) {
                product += innerRepresentation.ReferenceTime.benchmark_attribute;
                product += "\" \"" + innerRepresentation.ReferenceTime.value;
                product += "\" \"" + innerRepresentation.ReferenceTime.value_attribute;
            }
            else {
                product += "frequency\" \"2,8GHz\" \"" + innerRepresentation.ReferenceTime.value;
            }
            product += "\") )\n";
        }*/
        if (!innerRepresentation.File.empty() || !innerRepresentation.Directory.empty()) {
            bool set_Threads = false;
            int minimum;

            // all File elements checking
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                // checking the Source elements
                std::list<Arc::SourceType>::const_iterator it_source;
                for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                    if ( (*it_source).Threads > -1 && !set_Threads ) {
                       set_Threads = true;
                       minimum = (*it_source).Threads;
                    }
                    else if ( (*it_source).Threads > -1 && set_Threads && (*it_source).Threads < minimum ) {
                       minimum = (*it_source).Threads;
                    }
                }
                // checking the Target elements
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                    if ( (*it_target).Threads > -1 && !set_Threads ) {
                       set_Threads = true;
                       minimum = (*it_target).Threads;
                    }
                    else if ( (*it_target).Threads > -1 && set_Threads && (*it_target).Threads < minimum ) {
                       minimum = (*it_target).Threads;
                    }
                }
            }
            // all Directory elements checking
            std::list<Arc::DirectoryType>::const_iterator iter_d;
            for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
                // checking the Source elements
                std::list<Arc::SourceType>::const_iterator it_source;
                for (it_source = (*iter_d).Source.begin(); it_source != (*iter_d).Source.end(); it_source++){
                    if ( (*it_source).Threads > -1 && !set_Threads ) {
                       set_Threads = true;
                       minimum = (*it_source).Threads;
                    }
                    else if ( (*it_source).Threads > -1 && set_Threads && (*it_source).Threads < minimum ) {
                       minimum = (*it_source).Threads;
                    }
                }
                // checking the Target elements
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter_d).Target.begin(); it_target != (*iter_d).Target.end(); it_target++){
                    if ( (*it_target).Threads > -1 && !set_Threads ) {
                       set_Threads = true;
                       minimum = (*it_target).Threads;
                    }
                    else if ( (*it_target).Threads > -1 && set_Threads && (*it_target).Threads < minimum ) {
                       minimum = (*it_target).Threads;
                    }
                }
            }

            std::ostringstream oss;
            oss << minimum;
            if ( set_Threads ) product += "( ftpthreads = \"" + oss.str() +"\" )\n" ;
        }
        if (!innerRepresentation.File.empty() || !innerRepresentation.Directory.empty() ) {
            std::vector<std::string> indexing_services;
            indexing_services.clear();
            // Indexing services searching
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
	      if (iter->DataIndexingService)
                if ( find( indexing_services.begin(), indexing_services.end(),(*iter).DataIndexingService.fullstr() ) == indexing_services.end() )
                   if ((*iter).DataIndexingService.fullstr() != "")
                      indexing_services.push_back((*iter).DataIndexingService.fullstr());
            }
            std::list<Arc::DirectoryType>::const_iterator iter_d;
            for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
	      if (iter_d->DataIndexingService)
                if ( find( indexing_services.begin(), indexing_services.end(),(*iter_d).DataIndexingService.fullstr() ) == indexing_services.end() )
                   if ((*iter_d).DataIndexingService.fullstr() != "")
                      indexing_services.push_back((*iter_d).DataIndexingService.fullstr());
            }
            // XRSL output(s) generation
            std::vector<std::string>::const_iterator i_url;
            for (i_url = indexing_services.begin(); i_url != indexing_services.end(); i_url++){
                product += "( replicacollection = \"";
                product +=  *i_url;
                product += "\" )\n";
            }
        }
        // nodeaccess is not support on the GM side.
        // in question attribute
        /*if (innerRepresentation.InBound) {
            product += "( nodeaccess = \"inbound\")\n";
        }
        if (innerRepresentation.OutBound) {
            product += "( nodeaccess = \"outbound\")\n";
        }*/
        // gridTime is not support on the GM side.
        // in question attribute
        /*if (!innerRepresentation.ReferenceTime.value.empty() && 
             innerRepresentation.ReferenceTime.value_attribute.empty() && 
             innerRepresentation.ReferenceTime.benchmark_attribute.empty() && 
             innerRepresentation.TotalCPUTime == -1 && 
             innerRepresentation.TotalWallTime == -1 ) {
            product += "( gridtime = \"";
            product += innerRepresentation.ReferenceTime.value;
            product += "\" )\n";
        }*/
        if ( !innerRepresentation.XRSL_elements.empty() ) {
          std::map<std::string, std::string>::const_iterator it;
          for ( it = innerRepresentation.XRSL_elements.begin(); it != innerRepresentation.XRSL_elements.end(); it++ ) {
              product += "( ";
              product += it->first;
              product += " = \"";
              product += it->second;
              product += "\" )\n";
          }
        }
        logger.msg(DEBUG, "[XRSLParser] Converting to XRSL");
        return true;
    }

} // namespace Arc
