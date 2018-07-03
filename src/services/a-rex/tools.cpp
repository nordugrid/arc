#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include <arc/ws-addressing/WSA.h>

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

  //   primary:
  //      accepted|preprocessing|
  //      processing|processing-accepting|processing-queued|processing-running|
  //      postprocessing|terminal 
  //   attribute:
  //      validating|
  //      server-paused|
  //      client-paused|
  //      client-stagein-possible|
  //      client-stageout-possible|
  //      provisioning|
  //      deprovisioning|
  //      server-stagein|
  //      server-stageout|
  //      batch-suspend|
  //      app-running|
  //      preprocessing-cancel|
  //      processing-cancel|
  //      postprocessing-cancel|
  //      validation-failure|
  //      preprocessing-failure|
  //      processing-failure|
  //      postprocessing-failure|
  //      app-failure|
  //      expired 
  void convertActivityStatusES(const std::string& gm_state,std::string& primary_state,std::list<std::string>& state_attributes,bool failed,bool pending,const std::string& failedstate,const std::string& failedcause) {
    bool failed_set = false;
    bool canceled = (failedcause == "client");
    primary_state = "";
    if(gm_state == "ACCEPTED") {
      primary_state="accepted";
      state_attributes.push_back("client-stagein-possible");
    } else if(gm_state == "PREPARING") {
      primary_state="preprocessing";
      state_attributes.push_back("client-stagein-possible");
      state_attributes.push_back("server-stagein");
    } else if(gm_state == "SUBMIT") {
      primary_state="processing-accepting";
    } else if(gm_state == "INLRMS") {
      // Reporting job state as not started executing yet.
      // Because we have no more detailed information this
      // is probably safest solution.
      primary_state="processing-queued";
    } else if(gm_state == "FINISHING") {
      primary_state="postprocessing";
      state_attributes.push_back("client-stageout-possible");
      state_attributes.push_back("server-stageout");
    } else if(gm_state == "FINISHED") {
      primary_state="terminal";
      state_attributes.push_back("client-stageout-possible");
    } else if(gm_state == "DELETED") {
      primary_state="terminal";
      state_attributes.push_back("expired");
    } else if(gm_state == "CANCELING") {
      primary_state="processing";
    };
    if(failedstate == "ACCEPTED") {
      state_attributes.push_back("validation-failure");
      failed_set = true;
    } else if(failedstate == "PREPARING") {
      state_attributes.push_back(canceled?"preprocessing-cancel":"preprocessing-failure");
      failed_set = true;
    } else if(failedstate == "SUBMIT") {
      state_attributes.push_back(canceled?"processing-cancel":"processing-failure");
      failed_set = true;
    } else if(failedstate == "INLRMS") {
      state_attributes.push_back(canceled?"processing-cancel":"processing-failure");
      // Or maybe APP-FAILURE
      failed_set = true;
    } else if(failedstate == "FINISHING") {
      state_attributes.push_back(canceled?"postprocessing-cancel":"postprocessing-failure");
      failed_set = true;
    } else if(failedstate == "FINISHED") {
    } else if(failedstate == "DELETED") {
    } else if(failedstate == "CANCELING") {
    };
    if(primary_state == "terminal") {
      if(failed && !failed_set) {
        // Must put something to mark job failed
        state_attributes.push_back("app-failure");
      };
    };
    if(!primary_state.empty()) {
      if(pending) state_attributes.push_back("server-paused");
    };
  }

  // ActivityStatus
  //   Status
  //     [accepted|preprocessing|
  //      processing|processing-accepting|processing-queued|processing-running|
  //      postprocessing|terminal]
  //   Attribute 0- 
  //     [validating|
  //      server-paused|
  //      client-paused|
  //      client-stagein-possible|
  //      client-stageout-possible|
  //      provisioning|
  //      deprovisioning|
  //      server-stagein|
  //      server-stageout|
  //      batch-suspend|
  //      app-running|
  //      preprocessing-cancel|
  //      processing-cancel|
  //      postprocessing-cancel|
  //      validation-failure|
  //      preprocessing-failure|
  //      processing-failure|
  //      postprocessing-failure|
  //      app-failure|
  //      expired]
  //   Timestamp (dateTime)
  //   Description 0-1
  Arc::XMLNode addActivityStatusES(Arc::XMLNode pnode,const std::string& gm_state,Arc::XMLNode glue_xml,bool failed,bool pending,const std::string& failedstate,const std::string& failedcause) {
    std::string primary_state;
    std::list<std::string> state_attributes;
    std::string glue_state("");
    convertActivityStatusES(gm_state,primary_state,state_attributes,failed,pending,failedstate,failedcause);
    Arc::XMLNode state = pnode.NewChild("estypes:ActivityStatus");
    state.NewChild("estypes:Status") = primary_state;
    for(std::list<std::string>::iterator st = state_attributes.begin();
                  st!=state_attributes.end();++st) {
      state.NewChild("estypes:Attribute") = *st;
    };
    return state;
  }

  JobIDGeneratorARC::JobIDGeneratorARC(const std::string& endpoint):endpoint_(endpoint) {
  }

  void JobIDGeneratorARC::SetLocalID(const std::string& id) {
    id_ = id;
  }

  Arc::XMLNode JobIDGeneratorARC::GetGlobalID(Arc::XMLNode& pnode) {
    Arc::XMLNode node;
    if(!pnode) {
      Arc::NS ns;
      ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
      ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
      Arc::XMLNode(ns,"bes-factory:ActivityIdentifier").Exchange(pnode);
      node = pnode;
    } else {
      node = pnode.NewChild("bes-factory:ActivityIdentifier");
    };
    Arc::WSAEndpointReference identifier(node);
    // Make job's ID
    identifier.Address(endpoint_); // address of service
    identifier.ReferenceParameters().NewChild("a-rex:JobID")=id_;
    identifier.ReferenceParameters().NewChild("a-rex:JobSessionDir")=endpoint_+"/"+id_;
    return node;
  }

  std::string JobIDGeneratorARC::GetGlobalID(void) {
    Arc::XMLNode node;
    GetGlobalID(node);
    std::string jobid;
    node.GetDoc(jobid);
    std::string::size_type p = 0;
    // squeeze into 1 line
    while((p=jobid.find_first_of("\r\n",p)) != std::string::npos) jobid.replace(p,1," ");
    return jobid;
  }

  std::string JobIDGeneratorARC::GetManager(void) {
    return endpoint_;
  }

  std::string JobIDGeneratorARC::GetInterface(void) {
    return "org.nordugrid.xbes";
  }

  JobIDGeneratorES::JobIDGeneratorES(const std::string& endpoint):endpoint_(endpoint) {
  }

  void JobIDGeneratorES::SetLocalID(const std::string& id) {
    id_ = id;
  }

  Arc::XMLNode JobIDGeneratorES::GetGlobalID(Arc::XMLNode& pnode) {
    Arc::XMLNode node;
    if(!pnode) {
      Arc::NS ns;
      ns["estypes"]="http://www.eu-emi.eu/es/2010/12/types";
      Arc::XMLNode(ns,"estypes:ActivityID").Exchange(pnode);
      node = pnode;
    } else {
      node = pnode.NewChild("estypes:ActivityID");
    };
    node = id_;
    return node;
  }

  std::string JobIDGeneratorES::GetGlobalID(void) {
    return id_;
  }

  std::string JobIDGeneratorES::GetManager(void) {
    return endpoint_;
  }

  std::string JobIDGeneratorES::GetInterface(void) {
    return "org.ogf.glue.emies.activitycreation";
  }



  JobIDGeneratorINTERNAL::JobIDGeneratorINTERNAL(const std::string& endpoint):endpoint_(endpoint) {
  }
  
  void JobIDGeneratorINTERNAL::SetLocalID(const std::string& id) {
    id_ = id;
  }

  Arc::XMLNode JobIDGeneratorINTERNAL::GetGlobalID(Arc::XMLNode& pnode) {
    //To-do make something more sensible for INTERNAL plugin case
    Arc::XMLNode node;
    if(!pnode) {
      Arc::NS ns;
      ns["estypes"]="http://www.eu-emi.eu/es/2010/12/types";
      Arc::XMLNode(ns,"estypes:ActivityID").Exchange(pnode);
      node = pnode;
    } else {
      node = pnode.NewChild("estypes:ActivityID");
    };
    node = id_;
    return node;
  }
  
  std::string JobIDGeneratorINTERNAL::GetGlobalID(void) {
    return id_;
  }
  
  std::string JobIDGeneratorINTERNAL::GetManager(void) {
    return endpoint_;
  }
  
  std::string JobIDGeneratorINTERNALL::GetInterface(void) {
    return "org.nordugrid.internal";
  }

}

