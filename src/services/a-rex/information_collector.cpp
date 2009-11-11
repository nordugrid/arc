#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <glibmm.h>

#include <arc/Run.h>
#include <arc/wsrf/WSResourceProperties.h>
#include <arc/message/PayloadSOAP.h>

#include "ldif/LDIFtoXML.h"
#include "config/ngconfig.h"
#include "grid-manager/conf/environment.h"
#include "job.h"
#include "arex.h"

namespace ARex {

static void GetGlueStates(Arc::XMLNode infodoc,std::map<std::string,std::string>& states);

void ARexService::InformationCollector(void) {
  thread_count_.RegisterThread();
  for(;;) {
    // Run information provider
    std::string xml_str;
    int r = -1;
    {
      std::string cmd;
      cmd=nordugrid_libexec_loc()+"/CEinfo.pl --config "+nordugrid_config_loc();
      std::string stdin_str;
      std::string stderr_str;
      Arc::Run run(cmd);
      run.AssignStdin(stdin_str);
      run.AssignStdout(xml_str);
      run.AssignStderr(stderr_str);
      logger_.msg(Arc::VERBOSE,"Cluster information provider: %s",cmd);
      if(!run.Start()) {
      };
      if(!run.Wait(infoprovider_wakeup_period_*10)) {
        logger_.msg(Arc::WARNING,"Cluster information provider timeout: %u seconds",
                    infoprovider_wakeup_period_*10);
      } else {
        r = run.Result();
        if (r!=0) logger_.msg(Arc::WARNING,"Cluster information provider failed with exit status: %i",r);
      };
      logger_.msg(Arc::VERBOSE,"Cluster information provider log:\n%s",stderr_str);
    };
    if (r!=0) {
      logger_.msg(Arc::VERBOSE,"No new informational document assigned");
    } else {
      logger_.msg(Arc::DEBUG,"Obtained XML: %s",xml_str);
      Arc::XMLNode root(xml_str);
      if(root) {
        // Collect job states
        GetGlueStates(root,glue_states_);
        // Put result into container
        infodoc_.Arc::InformationContainer::Assign(root,true);
        logger_.msg(Arc::VERBOSE,"Assigned new informational document");
      } else {
        logger_.msg(Arc::ERROR,"Failed to create informational document");
      };
    }
    if(thread_count_.WaitOrCancel(infoprovider_wakeup_period_*1000)) break;
  };
  thread_count_.UnregisterThread();
}

bool ARexService::RegistrationCollector(Arc::XMLNode &doc) {
  //Arc::XMLNode root = infodoc_.Acquire();
  logger_.msg(Arc::DEBUG,"Passing service's information from collector to registrator");
  Arc::XMLNode empty(ns_, "RegEntry");
  empty.New(doc);

  doc.NewChild("SrcAdv");
  doc.NewChild("MetaSrcAdv");

  doc["SrcAdv"].NewChild("Type") = "org.nordugrid.execution.arex";
  doc["SrcAdv"].NewChild("EPR").NewChild("Address") = endpoint_;
  //doc["SrcAdv"].NewChild("SSPair");

  return true;
  //
  // TODO: filter information here.
  //Arc::XMLNode regdoc("<Service/>");
  //regdoc.New(doc);
  //doc.NewChild(root);
  //infodoc_.Release();
}

std::string ARexService::getID() {
  return "ARC:AREX";
}

static void GetGlueStates(Arc::XMLNode infodoc,std::map<std::string,std::string>& states) {
  std::string path = "Domains/AdminDomain/Services/ComputingService/ComputingEndpoint/ComputingActivities/ComputingActivity";
  // Obtaining all job descriptions
  Arc::XMLNodeList nodes = infodoc.Path(path);
  // Pulling ids and states
  for(Arc::XMLNodeList::iterator node = nodes.begin();node!=nodes.end();++node) {
    // Exract ID of job
    std::string id = (*node)["IDFromEndpoint"];
    if(id.empty()) id = (std::string)((*node)["ID"]);
    if(id.empty()) continue;
    std::string::size_type p = id.rfind('/');
    if(p != std::string::npos) id.erase(0,p+1);
    if(id.empty()) continue;
    Arc::XMLNode state_node = (*node)["State"];
    for(;(bool)state_node;++state_node) {
      std::string state  = (std::string)state_node;
      if(state.empty()) continue;
      // Look for nordugrid prefix
      if(strncmp("nordugrid:",state.c_str(),10) == 0) {
        // Remove prefix
        state.erase(0,10);
        // Store state under id
        states[id] = state;
      };
    };
  };
}

class PrefixedFilePayload: public Arc::PayloadRawInterface {
 private:
  std::string prefix_;
  std::string postfix_;
  int handle_;
  void* addr_;
  size_t length_;
 public:
  PrefixedFilePayload(const std::string& prefix,const std::string& postfix,int handle) {
    prefix_ = prefix;
    postfix_ = postfix;
    handle_ = handle;
    length_ = 0;
    if(handle != -1) {
      struct stat st;
      if(::fstat(handle,&st) == 0) {
        if(st.st_size > 0) {
          length_ = st.st_size;
          addr_ = ::mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,handle,0);
          if(!addr_) length_=0;
        };
      };
    };
  };
  ~PrefixedFilePayload(void) {
    if(addr_) ::munmap(addr_,length_);
    ::close(handle_);
  };
  virtual char operator[](Size_t pos) const {
    char* p = ((PrefixedFilePayload*)this)->Content(pos);
    if(!p) return 0;
    return *p;
  };
  virtual char* Content(Size_t pos = -1) {
    if(pos < prefix_.length()) return (char*)(prefix_.c_str() + pos);
    pos -= prefix_.length();
    if(pos < length_) return ((char*)(addr_) + pos);
    pos -= length_; 
    if(pos < postfix_.length()) return (char*)(postfix_.c_str() + pos);
    return NULL;
  };
  virtual Size_t Size(void) const {
    return (prefix_.length() + length_ + postfix_.length());
  };
  virtual char* Insert(Size_t pos = 0,Size_t size = 0) {
    return NULL;
  };
  virtual char* Insert(const char* s,Size_t pos = 0,Size_t size = -1) {
    return NULL;
  };
  virtual char* Buffer(unsigned int num = 0) {
    if(num == 0) return (char*)(prefix_.c_str());
    if(addr_) {
      if(num == 1) return (char*)addr_;
    } else {
      ++num;
    };
    if(num == 2) return (char*)(postfix_.c_str());
    return NULL;
  };
  virtual Size_t BufferSize(unsigned int num = 0) const {
    if(num == 0) return prefix_.length();
    if(addr_) {
      if(num == 1) return length_;
    } else {
      ++num;
    };
    if(num == 2) return postfix_.length();
    return 0;
  };
  virtual Size_t BufferPos(unsigned int num = 0) const {
    if(num == 0) return 0;
    if(addr_) {
      if(num == 1) return prefix_.length();
    } else {
      ++num;
    };
    if(num == 2) return (prefix_.length() + length_);
    return (prefix_.length() + length_ + postfix_.length());
  };
  virtual bool Truncate(Size_t size) { return false; };
};

OptimizedInformationContainer::OptimizedInformationContainer(void) {
  handle_=-1;
}

OptimizedInformationContainer::~OptimizedInformationContainer(void) {
}

int OptimizedInformationContainer::OpenDocument(void) {
  int h = -1;
  olock_.lock();
  if(handle_ != -1) h = ::dup(handle_);
  olock_.unlock();
  return h;
}

Arc::MessagePayload* OptimizedInformationContainer::Process(Arc::SOAPEnvelope& in) {
  Arc::WSRF& wsrp = Arc::CreateWSRP(in);
  if(!wsrp) { delete &wsrp; return NULL; };
  try {
    Arc::WSRPGetResourcePropertyDocumentRequest* req =
         dynamic_cast<Arc::WSRPGetResourcePropertyDocumentRequest*>(&wsrp);
    if(!req) throw std::exception();
    if(!(*req)) throw std::exception();
    // Request for whole document
    std::string fake_str("<fake>fake</fake>");
    Arc::XMLNode xresp(fake_str);
    Arc::WSRPGetResourcePropertyDocumentResponse resp(xresp);
    std::string rest_str;
    resp.SOAP().GetDoc(rest_str);
    std::string::size_type p = rest_str.find(fake_str);
    if(p == std::string::npos) throw std::exception();
    PrefixedFilePayload* outpayload = new PrefixedFilePayload(rest_str.substr(0,p),rest_str.substr(p+fake_str.length()),OpenDocument());
    delete &wsrp;
    return outpayload;
  } catch(std::exception& e) { };
  delete &wsrp;
  Arc::NS ns;
  Arc::SOAPEnvelope* out = InformationContainer::Process(in);
  if(!out) return NULL;
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns);
  out->Swap(*outpayload);
  delete out;
  return outpayload;
}

void OptimizedInformationContainer::AssignFile(const std::string& filename) {
  olock_.lock();
  if(!filename_.empty()) ::unlink(filename_.c_str());
  if(handle_ != -1) ::close(handle_);
  filename_ = filename;
  handle_ = -1;
  if(!filename_.empty()) {
    handle_ = ::open(filename_.c_str(),O_RDONLY);
    lock_.lock();
    doc_.ReadFromFile(filename_);
    lock_.unlock();
    Arc::InformationContainer::Assign(doc_,false);
  };
  olock_.unlock();
}

void OptimizedInformationContainer::Assign(const std::string& xml) {
  std::string filename;
  int h = Glib::file_open_tmp(filename);
  if(h == -1) return;
  for(std::string::size_type p = 0;p<xml.length();++p) {
    ssize_t l = ::write(h,xml.c_str()+p,xml.length()-p);
    if(l == -1) {
      ::unlink(filename.c_str());
      ::close(h);
      return;
    };
    p+=l;
  };
  AssignFile(filename);
  ::close(h);
}

}

