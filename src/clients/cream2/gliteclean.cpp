#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstdlib>

#include <arc/ArcLocation.h>
#include <arc/misc/OptionParser.h>

#include "cream_client.h"

int main(int argc, char* argv[]){

    setlocale(LC_ALL, "");

    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

    Arc::ArcLocation::Init(argv[0]);

    Arc::OptionParser options(istring("service_url id_file"));

    std::string config_path;
    options.AddOption('c', "config",
                      istring("path to config file"),
                      istring("path"), config_path);

    std::string debug;
    options.AddOption('d', "debug",
                      istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
                      istring("debuglevel"), debug);

    bool version = false;
    options.AddOption('v', "version", istring("print version information"),
                      version);

    std::list<std::string> params = options.Parse(argc, argv);

    if (!debug.empty())
        Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

    if (version) {
        std::cout << Arc::IString("%s version %s", "gliteclean", VERSION)
                  << std::endl;
        return 0;
    }

    try{
        if (params.size()!=2) {
            throw std::invalid_argument("Wrong number of arguments!");
        }
    
        //Create the CREAMClient object
        std::list<std::string>::iterator it = params.begin();
        Arc::URL url(*it++);
        std::string jobidfilestr = *it++;
        if(!url)
            throw std::invalid_argument("Can't parse specified service URL");
        Arc::MCCConfig cfg;
        if(config_path != "") cfg.GetOverlay(config_path);
        Arc::Cream::CREAMClient gLiteClient(url,cfg);
        
        // Read the jobid from the jobid file into string
        std::string jobid;
        std::ifstream jobidfile(jobidfilestr.c_str());
        if (!jobidfile)
            throw std::invalid_argument("Could not open " + jobidfilestr);
        getline(jobidfile, jobid);
        if (!jobid.compare(""))
            throw std::invalid_argument("Could not read Job ID from " + jobidfilestr);
        
        // Kill the job
        gLiteClient.purge(jobid);
        
        // Notify user about the success (Just if succeeded, otherwise exception was threw)
        std::cout << "The job was removed." << std::endl;
        return EXIT_SUCCESS;
    
    } catch (std::exception& e){
        
        // Notify the user about the failure
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
