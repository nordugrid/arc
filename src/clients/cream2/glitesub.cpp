// glitesub

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientTool.h>
#include <cstdlib>
#include "cream_client.h"

class GLiteSubTool: public Arc::ClientTool {
    public:
        std::string delegation_id;
        std::string config_path;
        GLiteSubTool(int argc,char* argv[]):Arc::ClientTool("glitesub") {
            ProcessOptions(argc,argv,"c:D:");
        };
    virtual void PrintHelp(void) {
        std::cout<<"glitesub [-c client_config][-D delegation_id] service_url jsdl_file id_file"<<std::endl;
        exit(1);
    };
    virtual bool ProcessOption(char option,char* option_arg) {
        switch(option) {
            case 'D': delegation_id=option_arg; break;
            case 'c': config_path=option_arg; break;
            default: {
                std::cerr<<"Error processing option: "<<(char)option<<std::endl;
                PrintHelp();
                return false;
            };
        };
        return true;
    };
};

int main(int argc, char* argv[]){
    // Processing the arguments
    GLiteSubTool tool(argc,argv);
    if(!tool) return EXIT_FAILURE;
    
    try{
        if ((argc-tool.FirstOption())!=3) {
            tool.PrintHelp();
            throw std::invalid_argument("Wrong number of arguments!");
        }
    
        //Create the CREAMClient object
        Arc::URL url(argv[tool.FirstOption()]);
        if(!url) throw(std::invalid_argument(std::string("Can't parse specified URL")));
        Arc::MCCConfig cfg;
        if(tool.config_path != "") cfg.GetOverlay(tool.config_path);
        Arc::Cream::CREAMClient gLiteClient(url,cfg);
        
        // Set delegation if necessary
        if (tool.delegation_id != "") gLiteClient.setDelegationId(tool.delegation_id);

        // Read the job description from file to string
        std::ifstream jsdlfile(argv[tool.FirstOption()+1]);
        std::string jsdl_text = "";
        
        if (!jsdlfile) throw std::invalid_argument(std::string("Could not open ") + argv[tool.FirstOption()+1]);
        std::string s;
        while(getline(jsdlfile, s)) jsdl_text +=  s + "\n"; // ... must add it back
        
        // Submit the job (registerJob; startJob)
        std::string jobid;
        jobid = gLiteClient.submit(jsdl_text);
        
        // Write the jobid into the given file
        std::ofstream jobidfile(argv[tool.FirstOption()+2]);
        jobidfile << jobid;
        if (!jobidfile) throw std::invalid_argument(std::string("Could not write Job ID to ") + std::string(argv[tool.FirstOption()+2]));
        
        // Notify the user about the success
        std::cout << "Submitted the job!" << std::endl;
        std::cout << "Job ID stored in: " << argv[tool.FirstOption()+2] << std::endl;
        return EXIT_SUCCESS;
    } catch (std::exception& e){
        
        // Notify the user about the failure
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
