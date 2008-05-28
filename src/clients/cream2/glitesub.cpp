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

    Arc::OptionParser options(istring("service_url jsdl_file id_file"));

    std::string delegation_id;
    options.AddOption('D', "delegation",
                      istring("delegation ID"),
                      istring("id"), delegation_id);

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
        std::cout << Arc::IString("%s version %s", "glitesub", VERSION)
                  << std::endl;
        return 0;
    }

    try{
        if (params.size()!=3) {
            throw std::invalid_argument("Wrong number of arguments!");
        }

        //Create the CREAMClient object
        std::list<std::string>::iterator it = params.begin();
        Arc::URL url(*it++);
        std::string jsdlfilestr = *it++;
        std::string jobidfilestr = *it++;
        if(!url)
            throw std::invalid_argument("Can't parse specified URL");
        Arc::MCCConfig cfg;
        if(config_path != "") cfg.GetOverlay(config_path);
        Arc::Cream::CREAMClient gLiteClient(url,cfg);
        
        // Set delegation if necessary
        if (delegation_id != "") gLiteClient.setDelegationId(delegation_id);

        // Read the job description from file to string
        std::ifstream jsdlfile(jsdlfilestr.c_str());
        std::string jsdl_text = "";
        
        if (!jsdlfile)
            throw std::invalid_argument("Could not open " + jsdlfilestr);
        std::string s;
        while(getline(jsdlfile, s)) jsdl_text +=  s + "\n"; // ... must add it back
        
        // Submit the job (registerJob; startJob)
        std::string jobid;
        jobid = gLiteClient.submit(jsdl_text);
        
        // Write the jobid into the given file
        std::ofstream jobidfile(jobidfilestr.c_str());
        jobidfile << jobid;
        if (!jobidfile)
            throw std::invalid_argument("Could not write Job ID to " + jobidfilestr);
        
        // Notify the user about the success
        std::cout << "Submitted the job!" << std::endl;
        std::cout << "Job ID stored in: " << jobidfilestr << std::endl;
        return EXIT_SUCCESS;
    } catch (std::exception& e){
        
        // Notify the user about the failure
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
