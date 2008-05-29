#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

#include <arc/ArcLocation.h>
#include <arc/XMLNode.h>
#include <arc/OptionParser.h>

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

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("service_url jsdl_file id_file"));

  std::string proxy_path;
  options.AddOption('P', "proxy", istring("path to proxy file"),
		    istring("path"), proxy_path);

  std::string cert_path;
  options.AddOption('C', "certifcate", istring("path to certificate file"),
		    istring("path"), cert_path);

  std::string key_path;
  options.AddOption('K', "key", istring("path to private key file"),
		    istring("path"), key_path);

  std::string ca_dir;
  options.AddOption('A', "cadir", istring("path to CA directory"),
		    istring("directory"), ca_dir);

  std::string config_path;
  options.AddOption('c', "config", istring("path to config file"),
		    istring("path"), config_path);

  bool delegate = false;
  options.AddOption('E', "delegate", istring("delegate proxy"), delegate);

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
    std::cout << Arc::IString("%s version %s", "apsub", VERSION) << std::endl;
    return 0;
  }

  try{
    if (params.size()!=3)
      throw std::invalid_argument("Wrong number of arguments!");
    std::list<std::string>::iterator it = params.begin();
    Arc::URL url(*it++);
    std::string jsdlfilename = *it++;
    std::string idfilename = *it++;
    if(!url)
      throw(std::invalid_argument(std::string("Can't parse specified URL")));
    Arc::MCCConfig cfg;
    if(!proxy_path.empty()) cfg.AddProxy(proxy_path);
    if(!key_path.empty()) cfg.AddPrivateKey(key_path);
    if(!cert_path.empty()) cfg.AddCertificate(cert_path);
    if(!ca_dir.empty()) cfg.AddCADir(ca_dir);
    cfg.GetOverlay(config_path);
    Arc::AREXClient ac(url,cfg);
    std::string jobid;
    std::ifstream jsdlfile(jsdlfilename.c_str());
    if (!jsdlfile)
      throw std::invalid_argument("Could not open job description file " + jsdlfilename);
    std::ofstream jobidfile(idfilename.c_str());
    Arc::AREXFileList files;
    // Submit job description
    jobid = ac.submit(jsdlfile,files,delegate);
    if (!jsdlfile)
      throw std::invalid_argument("Failed when reading from " + jsdlfilename);
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
      throw std::invalid_argument("Could not write Job ID to " + idfilename);
    std::cout << "Submitted the job!" << std::endl;
    std::cout << "Job ID stored in: " << idfilename << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& e){
    std::cerr << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
