#include <string>
#include <arc/XMLNode.h>

namespace ARex {

void convertActivityStatusES(const std::string& gm_state,std::string& primary_state,std::list<std::string>& state_attributes,bool failed,bool pending,const std::string& failedstate,const std::string& failedcause);

Arc::XMLNode addActivityStatusES(Arc::XMLNode pnode,const std::string& gm_state,bool failed,bool pending,const std::string& failedstate = "",const std::string& failedcause = "");

Arc::XMLNode addActivityStatusES(Arc::XMLNode pnode,Arc::XMLNode glue_xml);

class JobIDGenerator {
 public:
  JobIDGenerator() { };
  virtual ~JobIDGenerator() { };
  virtual void SetLocalID(const std::string& id) = 0;
  virtual Arc::XMLNode GetGlobalID(Arc::XMLNode& pnode) = 0;
  virtual std::string GetGlobalID(void) = 0;
  virtual std::string GetManager(void) = 0;
  virtual std::string GetInterface(void) = 0;
};

class JobIDGeneratorARC:public JobIDGenerator {
 public:
  JobIDGeneratorARC(const std::string& endpoint);
  virtual ~JobIDGeneratorARC() { };
  virtual void SetLocalID(const std::string& id);
  virtual Arc::XMLNode GetGlobalID(Arc::XMLNode& pnode);
  virtual std::string GetGlobalID(void);
  virtual std::string GetManager(void);
  virtual std::string GetInterface(void);
 private:
  std::string endpoint_;
  std::string id_;
};

class JobIDGeneratorES:public JobIDGenerator {
 public:
  JobIDGeneratorES(const std::string& endpoint);
  virtual ~JobIDGeneratorES() { };
  virtual void SetLocalID(const std::string& id);
  virtual Arc::XMLNode GetGlobalID(Arc::XMLNode& pnode);
  virtual std::string GetGlobalID(void);
  virtual std::string GetManager(void);
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
  virtual std::string GetManager(void);
  virtual std::string GetInterface(void);
 private:
  std::string endpoint_;
  std::string id_;
};

Arc::XMLNode addJobID(Arc::XMLNode& pnode,const std::string& endpoint,const std::string& id);
std::string makeJobID(const std::string& endpoint,const std::string& id);
Arc::XMLNode addJobIDES(Arc::XMLNode& pnode,const std::string& endpoint,const std::string& id);
std::string makeJobIDES(const std::string& endpoint,const std::string& id);
Arc::XMLNode addJobIDINTERNAL(Arc::XMLNode& pnode,const std::string& endpoint,const std::string& id);
std::string makeJobIDINTERNAL(const std::string& endpoint,const std::string& id);
 

}

