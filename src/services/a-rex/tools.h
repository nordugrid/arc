#include <string>
#include <arc/XMLNode.h>

namespace ARex {

void convertActivityStatus(const std::string& gm_state,std::string& bes_state,std::string& arex_state,bool failed = false,bool pending = false);
void addActivityStatus(Arc::XMLNode pnode,const std::string& gm_state,bool failed = false,bool pending = false);

}

