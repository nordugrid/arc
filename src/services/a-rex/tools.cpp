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

  void convertActivityStatusES(const std::string& gm_state,std::string& primary_state,std::list<std::string>& state_attributes,bool failed,bool pending) {
    primary_state = "";
    if(gm_state == "ACCEPTED") {
      primary_state="ACCEPTED";
    } else if(gm_state == "PREPARING") {
      primary_state="PREPROCESSING";
      state_attributes.push_back("CLIENT-STAGEIN-POSSIBLE");
      state_attributes.push_back("SERVER-STAGEIN");
    } else if(gm_state == "SUBMIT") {
      primary_state="PROCESSING-ACCEPTING";
    } else if(gm_state == "INLRMS") {
      //primary_state="PROCESSING-QUEUED"; TODO
      primary_state="PROCESSING-RUNNING";
      state_attributes.push_back("APP-RUNNING"); // TODO
      //state_attributes.push_back("BATCH-SUSPEND"); TODO
    } else if(gm_state == "FINISHING") {
      primary_state="POSTPROCESSING";
      state_attributes.push_back("CLIENT-STAGEOUT-POSSIBLE");
      state_attributes.push_back("SERVER-STAGEOUT");
    } else if(gm_state == "FINISHED") {
      primary_state="TERMINAL";
      state_attributes.push_back("CLIENT-STAGEOUT-POSSIBLE");
    } else if(gm_state == "DELETED") {
      primary_state="TERMINAL";
      state_attributes.push_back("CLIENT-STAGEOUT-POSSIBLE");
    } else if(gm_state == "CANCELING") {
      primary_state="PROCESSING";
    };
    if(primary_state == "TERMINAL") {
      if(failed) {
        //state_attributes.push_back("PREPROCESSING-CANCEL"); TODO
        //state_attributes.push_back("PROCESSING-CANCEL"); TODO
        //state_attributes.push_back("POSTPROCESSING-CANCEL"); TODO
        //state_attributes.push_back("VALIDATION-FAILURE"); TODO
        //state_attributes.push_back("PREPROCESSING-FAILURE"); TODO
        //state_attributes.push_back("PROCESSING-FAILURE"); TODO
        //state_attributes.push_back("POSTPROCESSING-FAILURE"); TODO
        state_attributes.push_back("APP-FAILURE"); // TODO
      };
    };
    if(!primary_state.empty()) {
      if(pending) state_attributes.push_back("SERVER-PAUSED");
    };
  }

  Arc::XMLNode addActivityStatusES(Arc::XMLNode pnode,const std::string& gm_state,Arc::XMLNode glue_xml,bool failed,bool pending) {
    std::string primary_state;
    std::list<std::string> state_attributes;
    std::string glue_state("");
    convertActivityStatusES(gm_state,primary_state,state_attributes,failed,pending);
    Arc::XMLNode state = pnode.NewChild("estypes:ActivityStatus");
    state.NewChild("estypes:Status") = primary_state;
    for(std::list<std::string>::iterator st = state_attributes.begin();
                  st!=state_attributes.end();++st) {
      state.NewChild("estypes:Attribute") = *st;
    };
    return state;
  }

}

