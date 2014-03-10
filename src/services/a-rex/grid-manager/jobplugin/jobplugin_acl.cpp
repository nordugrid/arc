#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Utils.h>
#include <arc/Logger.h>

#ifdef HAVE_GACL
#include "../../../gridftpd/auth/gacl_auth.h"
#include "../../../gridftpd/auth/permission_gacl.h"
#else
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>
#include <arc/message/SecAttr.h>
#endif

#include "jobplugin.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"JobPlugin");

#ifdef HAVE_GACL
int JobPlugin::check_acl(const char* acl_file,bool spec,const std::string& id) {
  int res = 0;
  GACLacl* acl = GACLloadAcl((char*)(acl_file));
  if(!acl) {
    logger.msg(Arc::ERROR, "Failed to read job's ACL for job %s from %s", id, config.ControlDir());
    return res;
  };
  GACLperm perm = AuthUserGACLTest(acl,user_a);
  if(spec) {
    if(GACLhasList(perm)) res|=IS_ALLOWED_LIST;
    if(GACLhasRead(perm) | GACLhasWrite(perm)) res|=(IS_ALLOWED_READ | IS_ALLOWED_LIST);
    if(GACLhasAdmin(perm)) res|=(IS_ALLOWED_READ | IS_ALLOWED_WRITE | IS_ALLOWED_LIST);
  } else {
    if(GACLhasList(perm)) res|=IS_ALLOWED_LIST;
    if(GACLhasRead(perm)) res|=IS_ALLOWED_READ;
    if(GACLhasWrite(perm)) res|=IS_ALLOWED_WRITE;
    if(GACLhasAdmin(perm)) res|=(IS_ALLOWED_READ | IS_ALLOWED_WRITE | IS_ALLOWED_LIST);
  };
  return res;
}

#else // HAVE_GACL

#define EVALUATE_ACTION(request,allowed_to,action_name) {\
  for(Arc::XMLNode entry = request["entry"];(bool)entry;++entry) {\
    entry["allow"].Destroy();\
    entry.NewChild("allow").NewChild(action_name);\
  };\
  ArcSec::Response *resp = eval->evaluate(request,policy.Ptr());\
  if(resp) {\
    ArcSec::ResponseList& rlist = resp->getResponseItems();\
    for(int n = 0; n<rlist.size(); ++n) {\
      ArcSec::ResponseItem* ritem = rlist[n];\
      if(!ritem) continue;\
      if(ritem->res != ArcSec::DECISION_PERMIT) continue;\
      allowed_to=true; break;\
    };\
  };\
}

int JobPlugin::check_acl(const char* acl_file,bool spec,const std::string& id) {
  int res = 0;
#ifdef USE_INTERNAL_GACL
  // TODO: this code is not complete yet
  // Identify and parse policy
  ArcSec::EvaluatorLoader eval_loader;
  Arc::AutoPointer<ArcSec::Policy> policy(eval_loader.getPolicy(ArcSec::SourceFile(acl_file)));
  if(!policy) {
    logger.msg(Arc::ERROR, "Failed to parse user policy for job %s", id);
    return res;
  };
  Arc::AutoPointer<ArcSec::Evaluator> eval(eval_loader.getEvaluator(policy.Ptr()));
  if(!eval) {
    logger.msg(Arc::VERBOSE, "Failed to load policy evaluator for policy of job %s", id);
    return res;
  };
  std::string policyname = policy->getName();
  if((policyname.length() > 7) &&
     (policyname.substr(policyname.length()-7) == ".policy")) {
    policyname.resize(policyname.length()-7);
  };
  if(policyname == "arc") {
    // TODO
  } else if(policyname == "gacl") {
    // Creating request - directly with XML
    Arc::NS ns;
    Arc::XMLNode request(ns,"gacl");
    bool allowed_to_list = false;
    bool allowed_to_read = false;
    bool allowed_to_write = false;
    bool allowed_to_admin = false;
    // Collect all security attributes
/*!!!!
    for(std::list<Arc::MessageAuth*>::iterator a = config_.beginAuth();a!=config_.endAuth();++a) {
      if(*a) (*a)->Export(Arc::SecAttr::GACL,request);
    };
*/
    // Leave only client identities
    int entries = 0;
    for(Arc::XMLNode entry = request["entry"];(bool)entry;++entry) {
      for(Arc::XMLNode a = entry["allow"];(bool)a;a=entry["allow"]) a.Destroy();
      for(Arc::XMLNode a = entry["deny"];(bool)a;a=entry["deny"]) a.Destroy();
      ++entries;
    };
    if(!entries) request.NewChild("entry");
    // Evaluate every action separately
    EVALUATE_ACTION(request,allowed_to_list,"list");
    EVALUATE_ACTION(request,allowed_to_read,"read");
    EVALUATE_ACTION(request,allowed_to_write,"write");
    EVALUATE_ACTION(request,allowed_to_admin,"admin");
    if(spec) {
      if(allowed_to_list) res|=IS_ALLOWED_LIST;
      if(allowed_to_read | allowed_to_write) res|=(IS_ALLOWED_READ | IS_ALLOWED_LIST);
      if(allowed_to_admin) res|=(IS_ALLOWED_READ | IS_ALLOWED_WRITE | IS_ALLOWED_LIST);
    } else {
      if(allowed_to_list) res|=IS_ALLOWED_LIST;
      if(allowed_to_read) res|=IS_ALLOWED_READ;
      if(allowed_to_write) res|=IS_ALLOWED_WRITE;
      if(allowed_to_admin) res|=(IS_ALLOWED_READ | IS_ALLOWED_WRITE | IS_ALLOWED_LIST);
    };
  } else {
    logger.msg(Arc::VERBOSE, "Unknown ACL policy %s for job %s", policyname, id);
  };
#endif // USE_INTERNAL_GACL
  return res;
}

#endif // HAVE_GACL

