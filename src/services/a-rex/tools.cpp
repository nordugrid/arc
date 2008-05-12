#include "tools.h"

namespace ARex {

void addActivityStatus(Arc::XMLNode pnode,const std::string& gm_state,bool failed) {
    std::string bes_state("");
    std::string arex_state("");
    if(gm_state == "ACCEPTED") {
      bes_state="Pending"; arex_state="Accepted";
    } else if(gm_state == "PREPARING") {
      bes_state="Running"; arex_state="Preparing";
    } else if(gm_state == "SUBMIT") {
      bes_state="Running"; arex_state="Submiting";
    } else if(gm_state == "INLRMS") {
      bes_state="Running"; arex_state="Executing";
    } else if(gm_state == "FINISHING") {
      bes_state="Running"; arex_state="Finishing";
    } else if(gm_state == "FINISHED") {
      bes_state="Finished"; arex_state="Finished";
      if(failed) bes_state="Failed";
    } else if(gm_state == "DELETED") {
      bes_state="Finished"; arex_state="Deleted";
      if(failed) bes_state="Failed";
    } else if(gm_state == "CANCELING") {
      bes_state="Running"; arex_state="Killing";
    };
    Arc::XMLNode state = pnode.NewChild("bes-factory:ActivityStatus");
    state.NewAttribute("bes-factory:state")=bes_state;
    state.NewChild("a-rex:state")=arex_state;
}

}

