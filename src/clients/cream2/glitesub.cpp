// glitesub

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientTool.h>
#include "cream_client.h"

/*int main(int argc, char* argv[]){
    std::string urlstr="CCCCC";
    std::string jobid="0111001101110";
    std::string jsdl_text = "Hello world";
    Arc::URL url(urlstr);
    Arc::MCCConfig cfg;
    Arc::Cream::CREAMClient ac(url,cfg);
    
    std::cout << "Job status: " << ac.submit(jsdl_text) << std::endl;
    std::cout << "Job status: " << ac.stat(jobid) << std::endl;
}*/

class GLiteSubTool: public Arc::ClientTool {
    public:
        GLiteSubTool(int argc,char* argv[]):Arc::ClientTool("glitesub") {
            ProcessOptions(argc,argv,"");
        };
    virtual void PrintHelp(void) {
        std::cout<<"glitesub service_url jsdl_file id_file"<<std::endl;
    };
    virtual bool ProcessOption(char option,char* option_arg) {
        std::cerr<<"Error processing option: "<<(char)option<<std::endl;
        PrintHelp();
        return false;
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
        Arc::Cream::CREAMClient gLiteClient(url,cfg);
        
        // Read the job description from file to string
        std::ifstream jsdlfile(argv[tool.FirstOption()+1]);
        std::string jsdl_text = "";
        
        if (!jsdlfile) throw std::invalid_argument(std::string("Could not open ") + std::string(argv[1]));
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
