#include "cream_client.h"

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientTool.h>

int main(int argc, char* argv[]){
    std::string urlstr="CCCCC";
    std::string jobid="0111001101110";
    Arc::URL url(urlstr);
    Arc::MCCConfig cfg;
    Arc::Cream::CREAMClient ac(url,cfg);
    std::cout << "Job status: " << ac.stat(jobid) << std::endl;
}