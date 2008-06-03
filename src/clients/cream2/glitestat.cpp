#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstdlib>

#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>

#include "CREAMClient.h"

int main(int argc, char* argv[]){

    setlocale(LC_ALL, "");

    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

    Arc::ArcLocation::Init(argv[0]);

    Arc::OptionParser options(istring("info_file"));

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
        std::cout << Arc::IString("%s version %s", "glitestat", VERSION)
                  << std::endl;
        return 0;
    }

    try{
        if (params.size()!=1) {
            throw std::invalid_argument("Wrong number of arguments!");
        }
        std::list<std::string>::iterator it = params.begin();
    
        // Read the job informations from file
        Arc::XMLNode jobInformation;
        if (!jobInformation.ReadFromFile(*it))
            throw std::invalid_argument("Could not open " + *it++);
        
        //Create the CREAMClient object
        Arc::URL url(jobInformation["CreamURL"]);
        if(!url)
            throw std::invalid_argument("Can't parse specified service URL");
        Arc::MCCConfig cfg;
        if(config_path != "") cfg.GetOverlay(config_path);
        Arc::Cream::CREAMClient gLiteClient(url,cfg);
        
        std::string jobid = jobInformation["JobId"];
        if (!jobid.compare(""))
            throw std::invalid_argument("Could not read Job ID from the specified information file.");
        
        // Get and print the job status
        std::cout << "Job status: " << gLiteClient.stat(jobid) << std::endl;
        return EXIT_SUCCESS;
    
    } catch (std::exception& e){
        
        // Notify the user about the failure
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
