// glitestat.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientTool.h>
#include "cream_client.h"

class GLiteSubTool: public Arc::ClientTool {
    public:
        std::string delegation_id;
        std::string config_path;

        GLiteSubTool(int argc,char* argv[]):Arc::ClientTool("glitestat") {
            this->delegation_id = "";
            ProcessOptions(argc,argv,"d:c:");
        };
    virtual void PrintHelp(void) {
        std::cout<<"glitestat [-d delegation_id] service_url id_file"<<std::endl;
    };
    virtual bool ProcessOption(char option,char* option_arg) {
        switch(option) {
            case 'c': this->config_path=option_arg; break;
            case 'd': this->delegation_id=option_arg;; break;
            default: {
                std::cerr<<"Error processing option: "<<(char)option<<std::endl;
                PrintHelp();
                return false;
            };
        };
    };
};

int main(int argc, char* argv[]){
    // Processing the arguments
    GLiteSubTool tool(argc,argv);
    if(!tool) return EXIT_FAILURE;
    
    try{
        if ((argc-tool.FirstOption())!=2) {
            tool.PrintHelp();
            throw std::invalid_argument("Wrong number of arguments!");
        }
    
        //Create the CREAMClient object
        Arc::URL url(argv[tool.FirstOption()]);
        if(!url) throw(std::invalid_argument(std::string("Can't parse specified service URL")));
        Arc::MCCConfig cfg;
        if(tool.config_path != "") cfg.GetOverlay(tool.config_path);
        Arc::Cream::CREAMClient gLiteClient(url,cfg);
        
        // Set delegation if necessary
        if (tool.delegation_id != "") gLiteClient.setDelegationId(tool.delegation_id);
        
        // Read the jobid from the jobid file into string
        std::string jobid;
        std::ifstream jobidfile(argv[tool.FirstOption()+1]);
        if (!jobidfile) throw std::invalid_argument(std::string("Could not open ") + argv[tool.FirstOption()+1]);
        getline(jobidfile, jobid);
        if (!jobid.compare("")) throw std::invalid_argument(std::string("Could not read Job ID from ") + argv[tool.FirstOption()+1]);
        
        // Get and print the job status
        std::cout << "Job status: " << gLiteClient.stat(jobid) << std::endl;
        return EXIT_SUCCESS;
    
    } catch (std::exception& e){
        
        // Notify the user about the failure
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
