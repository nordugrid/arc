#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>

#include "SubmitterARC1.h"
#include "arex_client.h"

namespace Arc {

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
  std::cout << file.local << std::endl;
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

Arc::Logger SubmitterARC1::logger(Arc::Logger::rootLogger, "A-REX-Submitter");

static void set_arex_namespaces(Arc::NS& ns) {
  ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
  ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
  ns["wsa"]="http://www.w3.org/2005/08/addressing";
  ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";    
  ns["jsdl-posix"]="http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
  ns["jsdl-arc"]="http://www.nordugrid.org/ws/schemas/jsdl-arc";
  ns["jsdl-hpcpa"]="http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
}
  
  SubmitterARC1::SubmitterARC1(Config *cfg)
    : Submitter(cfg) {}
  
  SubmitterARC1::~SubmitterARC1() {}

  ACC *SubmitterARC1::Instance(Config *cfg, ChainContext *) {
    return new SubmitterARC1(cfg);
  }

  std::pair<URL, URL> SubmitterARC1::Submit(Arc::JobDescription& jobdesc) {
	   
    MCCConfig cfg;
    AREXClient ac(SubmissionEndpoint, cfg);

    std::string jobdescstring;
    jobdesc.getProduct(jobdescstring, "JSDL");
    std::istringstream jsdlfile(jobdescstring);

    AREXFileList files;
    std::string jobid = ac.submit(jsdlfile, files, false);

    XMLNode jobidx(jobid);
    URL session_url((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));

    // Upload user files
    for (AREXFileList::iterator file = files.begin();
	 file != files.end(); ++file) {
      if(!put_file(*(client), session_url, *file))
	throw std::invalid_argument(std::string("Could not upload file ") +
				    file->local + " to " + file->remote);
    };

    std::cout << "Submitted the job!" << std::endl;

    return std::make_pair(session_url, SubmissionEndpoint);

  } // SubmitterARC1::Submit    

} // namespace Arc
