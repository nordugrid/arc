#include <string>
#include <arc/XMLNode.h>

namespace ARex {

void convertActivityStatus(const std::string& gm_state,std::string& bes_state,std::string& arex_state,bool failed = false,bool pending = false);

Arc::XMLNode addActivityStatus(Arc::XMLNode pnode,const std::string& gm_state,Arc::XMLNode glue_xml = Arc::XMLNode(),bool failed = false,bool pending = false);

void convertActivityStatusES(const std::string& gm_state,std::string& primary_state,std::list<std::string>& state_attributes,bool failed,bool pending,const std::string& failedstate,const std::string& failedcause);

Arc::XMLNode addActivityStatusES(Arc::XMLNode pnode,const std::string& gm_state,Arc::XMLNode glue_xml = Arc::XMLNode(),bool failed = false,bool pending = false,const std::string& failedstate = "",const std::string& failedcause = "");

Arc::XMLNode addJobID(Arc::XMLNode& pnode,const std::string& endpoint,const std::string& id);
std::string makeJobID(const std::string& endpoint,const std::string& id);

Arc::XMLNode addJobIDES(Arc::XMLNode& pnode,const std::string& endpoint,const std::string& id);
std::string makeJobIDES(const std::string& endpoint,const std::string& id);

}

