#include <cstring>

#include "tools.h"

namespace ARex {

  void convertActivityStatus(const std::string& gm_state,std::string& bes_state,std::string& arex_state,bool failed,bool pending) {
    if(gm_state == "ACCEPTED") {
      bes_state="Pending"; arex_state="Accepted";
    } else if(gm_state == "PREPARING") {
      bes_state="Running"; arex_state=(!pending)?"Preparing":"Prepared";
    } else if(gm_state == "SUBMIT") {
      bes_state="Running"; arex_state="Submitting";
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
  }

  Arc::XMLNode addActivityStatus(Arc::XMLNode pnode,const std::string& gm_state,Arc::XMLNode glue_xml,bool failed,bool pending) {
    std::string bes_state("");
    std::string arex_state("");
    std::string glue_state("");
    convertActivityStatus(gm_state,bes_state,arex_state,failed,pending);
    Arc::XMLNode state = pnode.NewChild("bes-factory:ActivityStatus");
    state.NewAttribute("state")=bes_state;
    state.NewChild("a-rex:State")=arex_state;
    if(pending) state.NewChild("a-rex:State")="Pending";
    if((bool)glue_xml) {
      Arc::XMLNode state_node = glue_xml["State"];
      for(;(bool)state_node;++state_node) {
        std::string state  = (std::string)state_node;
        if(state.empty()) continue;
        // Look for nordugrid prefix
        if(::strncmp("nordugrid:",state.c_str(),10) == 0) {
          // Remove prefix
          state.erase(0,10);
          glue_state = state;
        };
      };
    };
    if(!glue_state.empty()) {
      std::string::size_type p = glue_state.find(':');
      if(p != std::string::npos) {
        if(glue_state.substr(0,p) == "INLRMS") {
          // Extrach state of batch system
          state.NewChild("a-rex:LRMSState")=glue_state.substr(p+1);
        };
      };
      state.NewChild("glue:State")=glue_state;
    };
    return state;
  }

}

