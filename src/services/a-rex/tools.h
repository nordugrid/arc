#ifndef __ARC_AREX_TOOLS_H__
#define __ARC_AREX_TOOLS_H__

#include <string>
#include <arc/XMLNode.h>

namespace ARex {

void convertActivityStatus(const std::string& gm_state,std::string& bes_state,std::string& arex_state,bool failed = false,bool pending = false);

Arc::XMLNode addActivityStatus(Arc::XMLNode pnode,const std::string& gm_state,Arc::XMLNode glue_xml = Arc::XMLNode(),bool failed = false,bool pending = false);

void convertActivityStatusES(const std::string& gm_state,std::string& primary_state,std::list<std::string>& state_attributes,bool failed,bool pending,const std::string& failedstate,const std::string& failedcause);

Arc::XMLNode addActivityStatusES(Arc::XMLNode pnode,const std::string& gm_state,Arc::XMLNode glue_xml = Arc::XMLNode(),bool failed = false,bool pending = false,const std::string& failedstate = "",const std::string& failedcause = "");

class JobIDGenerator {
 public:
  JobIDGenerator() { };
  virtual ~JobIDGenerator() { };
  virtual void SetLocalID(const std::string& id) = 0;
  virtual Arc::XMLNode GetGlobalID(Arc::XMLNode& pnode) = 0;
  virtual std::string GetGlobalID(void) = 0;
  virtual std::string GetJobURL(void) = 0;
  virtual std::string GetManagerURL(void) = 0;
  virtual std::string GetHostname(void) = 0;
  virtual std::string GetInterface(void) = 0;
};

/*
class JobIDGeneratorARC:public JobIDGenerator {
 public:
  JobIDGeneratorARC(const std::string& endpoint);
  virtual ~JobIDGeneratorARC() { };
  virtual void SetLocalID(const std::string& id);
  virtual Arc::XMLNode GetGlobalID(Arc::XMLNode& pnode);
  virtual std::string GetGlobalID(void);
  virtual std::string GetJobURL(void);
  virtual std::string GetManagerURL(void);
  virtual std::string GetHostname(void);
  virtual std::string GetInterface(void);
 private:
  std::string endpoint_;
  std::string id_;
};
*/

class JobIDGeneratorES:public JobIDGenerator {
 public:
  JobIDGeneratorES(const std::string& endpoint);
  virtual ~JobIDGeneratorES() { };
  virtual void SetLocalID(const std::string& id);
  virtual Arc::XMLNode GetGlobalID(Arc::XMLNode& pnode);
  virtual std::string GetGlobalID(void);
  virtual std::string GetJobURL(void);
  virtual std::string GetManagerURL(void);
  virtual std::string GetHostname(void);
  virtual std::string GetInterface(void);
 private:
  std::string endpoint_;
  std::string id_;
};
class JobIDGeneratorINTERNAL:public JobIDGenerator {
 public:
  JobIDGeneratorINTERNAL(const std::string& endpoint);
  virtual ~JobIDGeneratorINTERNAL() { };
  virtual void SetLocalID(const std::string& id);
  virtual Arc::XMLNode GetGlobalID(Arc::XMLNode& pnode);
  virtual std::string GetGlobalID(void);
  virtual std::string GetJobURL(void);
  virtual std::string GetManagerURL(void);
  virtual std::string GetHostname(void);
  virtual std::string GetInterface(void);
 private:
  std::string endpoint_;
  std::string id_;
};

class JobIDGeneratorREST:public JobIDGeneratorES {
 public:
  JobIDGeneratorREST(const std::string& endpoint):JobIDGeneratorES(endpoint) {};
  virtual std::string GetInterface(void);
};

Arc::XMLNode addJobID(Arc::XMLNode& pnode,const std::string& endpoint,const std::string& id);
std::string makeJobID(const std::string& endpoint,const std::string& id);
Arc::XMLNode addJobIDES(Arc::XMLNode& pnode,const std::string& endpoint,const std::string& id);
std::string makeJobIDES(const std::string& endpoint,const std::string& id);
Arc::XMLNode addJobIDINTERNAL(Arc::XMLNode& pnode,const std::string& endpoint,const std::string& id);
std::string makeJobIDINTERNAL(const std::string& endpoint,const std::string& id);
 

}


#endif // __ARC_AREX_TOOLS_H__

