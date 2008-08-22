#include "tools.h"

namespace ARex {

void addActivityStatus(Arc::XMLNode pnode,const std::string& gm_state,bool failed,bool pending) {
    std::string bes_state("");
    std::string arex_state("");
    if(gm_state == "ACCEPTED") {
      bes_state="Pending"; arex_state="Accepted";
    } else if(gm_state == "PREPARING") {
      bes_state="Running"; arex_state=(!pending)?"Preparing":"Prepared";
    } else if(gm_state == "SUBMIT") {
      bes_state="Running"; arex_state="Submiting";
    } else if(gm_state == "INLRMS") {
      bes_state="Running"; arex_state=(!pending)?"Executing":"Executed";
    } else if(gm_state == "FINISHING") {
      bes_state="Running"; arex_state="Finishing";
    } else if(gm_state == "FINISHED") {
      if(!failed) {
        bes_state="Finished"; arex_state="Finished";
      } else {
        bes_state="Failed"; arex_state="Failed";
      };
    } else if(gm_state == "DELETED") {
      // AFAIR failed is not avialable anymore.
      bes_state=(!failed)?"Finished":"Failed"; arex_state="Deleted";
    } else if(gm_state == "CANCELING") {
      bes_state="Running"; arex_state="Killing";
    };
    Arc::XMLNode state = pnode.NewChild("bes-factory:ActivityStatus");
    state.NewAttribute("state")=bes_state;
    state.NewChild("a-rex:state")=arex_state;
    if(pending) state.NewChild("a-rex:state")="Pending";
}

}

