#include "SubmitterARC1.h"

#include <iostream>
#include <stdlib.h>
#include <string>

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
/*  const uint64_t chunk_len = 1024*1024; // Some reasonable size - TODO - make ir configurable
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
  }; */
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
  : Submitter(cfg) {
	  
	  logger.msg(Arc::INFO, "Creating an A-REX client.");
	  client_loader = new Arc::Loader(cfg);
	  logger.msg(Arc::INFO, "Client side MCCs are loaded.");
	  client_entry = (*client_loader)["soap"];
	  if(!client_entry) {
		  logger.msg(Arc::ERROR, "Client chain does not have entry point.");
	  }
	  //client = new Arc::ClientSOAP(cfg,url.Host(),url.Port(),url.Protocol() == "https",url.Path());
	  set_arex_namespaces(arex_ns);
  }
  
  SubmitterARC1::~SubmitterARC1() {
	    if(client_loader) delete client_loader;
	    if(client_config) delete client_config;
	    if(client) delete client;
  }

  ACC *SubmitterARC1::Instance(Config *cfg, ChainContext *) {
    return new SubmitterARC1(cfg);
  }

  std::pair<URL, URL> SubmitterARC1::Submit(const std::string& jobdesc) {
	   
	  try{
		 
		// TODO: Move the InfoEndpoint URL to the final source code place 
		  
	    Arc::URL InfoEndpoint("https://knowarc1.grid.niif.hu/arex");
	    std::string jobid, faultstring;
	    Arc::XMLNode jsdl_document;
	    (Arc::XMLNode (jobdesc)).New(jsdl_document);
	    
	    AREXFileList file_list;
	    file_list.resize(0);

	    logger.msg(Arc::INFO, "Creating and sending request.");

	    // Create job request
	    /*
	      bes-factory:CreateActivity
	        bes-factory:ActivityDocument
	          jsdl:JobDefinition
	    */
	    Arc::PayloadSOAP req(arex_ns);
	    Arc::XMLNode op = req.NewChild("bes-factory:CreateActivity");
	    Arc::XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
	    act_doc.NewChild(jsdl_document);
	    act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces
	    Arc::PayloadSOAP* resp = NULL;

	    XMLNode ds = act_doc["jsdl:JobDefinition"]["jsdl:JobDescription"]["jsdl:DataStaging"];
	    for(;(bool)ds;ds=ds[1]) {
	      // FilesystemName - ignore
	      // CreationFlag - ignore
	      // DeleteOnTermination - ignore
	      XMLNode source = ds["jsdl:Source"];
	      XMLNode target = ds["jsdl:Target"];
	      if((bool)source) {
	        std::string s_name = ds["jsdl:FileName"];
	        if(!s_name.empty()) {
	          XMLNode x_url = source["jsdl:URI"];
	          std::string s_url = x_url;
	          if(s_url.empty()) {
	            s_url="./"+s_name;
	          } else {
	            URL u_url(s_url);
	            if(!u_url) {
	              if(s_url[0] != '/') s_url="./"+s_url;
	            } else {
	              if(u_url.Protocol() == "file") {
	                s_url=u_url.Path();
	                if(s_url[0] != '/') s_url="./"+s_url;
	              } else {
	                s_url.resize(0);
	              };
	            };
	          };
	          if(!s_url.empty()) {
	            x_url.Destroy();
	            AREXFile file(s_name,s_url);
	            file_list.push_back(file);
	          };
	        };
	      };
	    };
	    
	    std::string jsdl_str;
	    
	    act_doc.GetXML(jsdl_str);
	    logger.msg(Arc::VERBOSE, "Job description to be sent: %s",jsdl_str);

	    if (client_entry) {
	      Arc::Message reqmsg;
	      Arc::Message repmsg;
	      Arc::MessageAttributes attributes_req;
	      attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/CreateActivity");
	      Arc::MessageAttributes attributes_rep;
	      Arc::MessageContext context;
	      reqmsg.Payload(&req);
	      reqmsg.Attributes(&attributes_req);
	      reqmsg.Context(&context);
	      repmsg.Attributes(&attributes_rep);
	      repmsg.Context(&context);
	      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
	      if(!status) {
	        logger.msg(Arc::ERROR, "Submission request failed.");
	      }
	      logger.msg(Arc::INFO, "Submission request succeed.");
	      if(repmsg.Payload() == NULL) {
	        logger.msg(Arc::ERROR, "There were no response to a submission request.");	        
	      }
	      try {
	        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
	      } catch(std::exception&) { };
	      if(resp == NULL) {
	        logger.msg(Arc::ERROR,"A response to a submission request was not a SOAP message.");
	        delete repmsg.Payload();
	      };
	    } else {
	      
	    }
	    
	    Arc::XMLNode id, fs;
	    (*resp)["CreateActivityResponse"]["ActivityIdentifier"].New(id);
	    (*resp)["Fault"]["faultstring"].New(fs);
	    id.GetDoc(jobid);
	    faultstring=(std::string)fs;
	    delete resp;
	    if (faultstring=="") {
	      try {
		    Arc::XMLNode jobidx(jobid);
		    Arc::URL session_url((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));
		    if(!session_url)
		      throw std::invalid_argument(std::string("Could not extract session URL from job id: ") +
		                                    jobid);
		    // Upload user files
		    for(Arc::AREXFileList::iterator file = file_list.begin();file!=file_list.end();++file) {
		      std::cout<<"Uploading file "<<file->local<<" to "<<file->remote<<std::endl;
		      if(!put_file(*(client),session_url,*file))
		        throw std::invalid_argument(std::string("Could not upload file ") +
		                                    file->local + " to " + file->remote);
		    };

		    std::cout << "Submitted the job!" << std::endl;

		    return std::make_pair(session_url, InfoEndpoint);

		  }
		  catch (std::exception& e){
		    std::cerr << "ERROR: " << e.what() << std::endl;
		  }
	   }
	    else 
	      // TODO: error handling
	    	;
	  }  
	  catch (std::exception& e){
		  std::cerr << "ERROR: " << e.what() << std::endl;  
	  }
	  
	} // SubmitterARC1::Submit    
} // namespace Arc
