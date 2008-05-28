// apsub.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientTool.h>
#include "arex_client.h"

static std::string merge_paths(const std::string path1,const std::string& path2) {
  std::string path = path1;
  if((path.empty()) || (path[path.length()-1] != '/')) path+="/";
  if(path2[0] != '/') {
    path+=path2;
  } else {
    path+=path2.substr(1);
  };
  return path;
}

static bool put_file(Arc::ClientHTTP& client,const Arc::URL& base,Arc::AREXFile& file) {
  Arc::URL url = base;
  url.ChangePath(merge_paths(url.Path(),file.remote));
  std::string file_name = file.local; // Relative to current directory
  const uint64_t chunk_len = 1024*1024; // Some reasonable size - TODO - make ir configurable
  uint64_t chunk_start = 0;
  uint64_t chunk_end = chunk_len;
  std::ifstream f(file_name.c_str());
  f.seekg(0,std::ios::end);
  uint64_t file_size = f.tellg();
  f.seekg(0,std::ios::beg);
  if(!f) return false;
  for(;;) {
    Arc::PayloadRawInterface* resp;
    // TODO: Use PayloadFile instead of reaing file chunks to memory
    // Arc::PayloadFile req(fname,chunk_start,chunk_end);
    Arc::PayloadRaw req;
    char* file_buf = req.Insert(chunk_start,chunk_end-chunk_start);
    if(!file_buf) return false;
    f.read(file_buf,chunk_end-chunk_start);
    req.Truncate(file_size);
    chunk_end = chunk_start+f.gcount();
    if(chunk_start == chunk_end) break; // EOF
    Arc::HTTPClientInfo info;
    Arc::MCC_Status r = client.process("PUT",url.Path(),&req,&info,&resp);
    if(!resp) return false;
    if(info.code != 200) { delete resp; return false; };
    chunk_start=chunk_end; chunk_end=chunk_start+chunk_len;
  };
  return true;
}

class APSubTool: public Arc::ClientTool {
 public:
  std::string proxy_path;
  std::string key_path;
  std::string cert_path;
  std::string ca_dir;
  std::string config_path;
  bool delegate;
  APSubTool(int argc,char* argv[]):Arc::ClientTool("apsub") {
    delegate=false;
    ProcessOptions(argc,argv,"EP:K:C:A:c:");
  };
  virtual void PrintHelp(void) {
    std::cout<<"apsub [-h] [-d debug_level] [-l logfile] [-P proxy_path] [-C certificate_path] [-K private_key_path] [-A CA_directory_path] [-c config_path] [-E] service_url jsdl_file id_file"<<std::endl;
    std::cout<<"\tPossible debug levels are VERBOSE, DEBUG, INFO, WARNING, ERROR and FATAL"<<std::endl;
  };
  virtual bool ProcessOption(char option,char* option_arg) {
    switch(option) {
      case 'P': proxy_path=option_arg;; break;
      case 'K': key_path=option_arg; break;
      case 'C': cert_path=option_arg; break;
      case 'A': ca_dir=option_arg; break;
      case 'c': config_path=option_arg; break;
      case 'E': delegate=true; break;
      default: {
        std::cerr<<"Error processing option: "<<(char)option<<std::endl;
        PrintHelp();
        return false;
      };
    };
    return true;
  };
};


//! A prototype client for job submission.
/*! A prototype command line tool for job submission to an A-REX
  service. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apsub <Service-URL> <JSDL-file> <JobID-file>

  Arguments:
  <JSDL-file> The name of a file that contains the job specification
  in JSDL format.
  <JobID-file> The name of a file in which the Job ID will be stored.

*/
int main(int argc, char* argv[]){
  APSubTool tool(argc,argv);
  if(!tool) return EXIT_FAILURE;
  try{
    if ((argc-tool.FirstOption())!=3)
      throw std::invalid_argument("Wrong number of arguments!");
    Arc::URL url(argv[tool.FirstOption()]);
    if(!url) throw(std::invalid_argument(std::string("Can't parse specified URL")));
    Arc::MCCConfig cfg;
    if(!tool.proxy_path.empty()) cfg.AddProxy(tool.proxy_path);
    if(!tool.key_path.empty()) cfg.AddPrivateKey(tool.key_path);
    if(!tool.cert_path.empty()) cfg.AddCertificate(tool.cert_path);
    if(!tool.ca_dir.empty()) cfg.AddCADir(tool.ca_dir);
    cfg.GetOverlay(tool.config_path);
    Arc::AREXClient ac(url,cfg);
    std::string jobid;
    std::ifstream jsdlfile(argv[tool.FirstOption()+1]);
    if (!jsdlfile)
      throw std::invalid_argument(std::string("Could not open job description file ")+
				  std::string(argv[tool.FirstOption()+1]));
    std::ofstream jobidfile(argv[tool.FirstOption()+2]);
    Arc::AREXFileList files;
    // Submit job description
    jobid = ac.submit(jsdlfile,files,tool.delegate);
    if (!jsdlfile)
      throw std::invalid_argument(std::string("Failed when reading from ")+
				  std::string(argv[tool.FirstOption()+1]));
    // Extract session directory URL
    // TODO: use XML directly
    Arc::XMLNode jobidx(jobid);
    Arc::URL session_url((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));
    if(!session_url)
      throw std::invalid_argument(std::string("Could not extract session URL from job id: ") +
                                    jobid);
    // Upload user files
    for(Arc::AREXFileList::iterator file = files.begin();file!=files.end();++file) {
      std::cout<<"Uploading file "<<file->local<<" to "<<file->remote<<std::endl;
      if(!put_file(*(ac.SOAP()),session_url,*file))
        throw std::invalid_argument(std::string("Could not upload file ") +
                                    file->local + " to " + file->remote);
    };
    jobidfile << jobid;
    if (!jobidfile)
      throw std::invalid_argument(std::string("Could not write Job ID to ")+
				  std::string(argv[tool.FirstOption()+2]));
    std::cout << "Submitted the job!" << std::endl;
    std::cout << "Job ID stored in: " << argv[tool.FirstOption()+2] << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& e){
    std::cerr << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
