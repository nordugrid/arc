// glitedelegate.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientTool.h>
#include "cream_client.h"

class GLiteSubTool: public Arc::ClientTool {
    public:
        std::string config_path;
        GLiteSubTool(int argc,char* argv[]):Arc::ClientTool("glitedelegate") {
            ProcessOptions(argc,argv,"c:");
        };
    virtual void PrintHelp(void) {
        std::cout<<"glitedelegate delegation_id service_url"<<std::endl;
    };
    virtual bool ProcessOption(char option,char* option_arg) {
        switch(option) {
            case 'c': this->config_path=option_arg; break;
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
        Arc::URL url(argv[tool.FirstOption()+1]);
        if(!url) throw(std::invalid_argument(std::string("Can't parse specified service URL")));
        Arc::MCCConfig cfg;
        if(tool.config_path != "") cfg.GetOverlay(tool.config_path);
        Arc::Cream::CREAMClient gLiteClient(url,cfg);
        
        // Get the delegationid from the command line arguments
        std::string delegation_id = argv[tool.FirstOption()];
        
        // Create delegation
        gLiteClient.createDelegation(delegation_id);
        
        // Notify user about the success (Just if succeeded, otherwise exception was threw)
        std::cout << "Delegation has been created." << std::endl;
        return EXIT_SUCCESS;
    
    } catch (std::exception& e){
        
        // Notify the user about the failure
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
