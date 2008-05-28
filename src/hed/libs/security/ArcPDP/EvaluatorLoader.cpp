#ifdef HAVE_CONFIG_H
#include <config.h> 
#endif

#include <iostream>
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/loader/ClassLoader.h>
#include <arc/security/ArcPDP/Evaluator.h>

#include "EvaluatorLoader.h"

Arc::Logger ArcSec::EvaluatorLoader::logger(Arc::Logger::rootLogger, "EvaluatorLoader");

namespace ArcSec {
  Arc::XMLNode arc_evaluator_cfg_nd("\
    <ArcConfig\
     xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\
     xmlns:pdp=\"http://www.nordugrid.org/schemas/pdp/Config\">\
     <ModuleManager>\
        <Path></Path>\
     </ModuleManager>\
     <Plugins Name='arcpdc'>\
          <Plugin Name='__arc_attrfactory_modules__'>attrfactory</Plugin>\
          <Plugin Name='__arc_fnfactory_modules__'>fnfactory</Plugin>\
          <Plugin Name='__arc_algfactory_modules__'>algfactory</Plugin>\
          <Plugin Name='__arc_evaluator_modules__'>evaluator</Plugin>\
          <Plugin Name='__arc_request_modules__'>request</Plugin>\
          <Plugin Name='__arc_policy_modules__'>policy</Plugin>\
     </Plugins>\
     <pdp:PDPConfig>\
          <pdp:AttributeFactory name='attr.factory' />\
          <pdp:CombingAlgorithmFactory name='alg.factory' />\
          <pdp:FunctionFactory name='fn.factory' />\
          <pdp:Evaluator name='arc.evaluator' />\
          <pdp:Request name='arc.request' />\
          <pdp:Policy name='arc.policy' />\
     </pdp:PDPConfig>\
    </ArcConfig>");

  //Arc::XMLNode xacml_evaluator_cfg_nd

EvaluatorLoader::EvaluatorLoader() {
  class_config_list_.push_back(arc_evaluator_cfg_nd);
  //class_config_map_.push_back(xacml_evaluator_cfg_nd);
}

Evaluator* EvaluatorLoader::getEvaluator(std::string& classname) {
  ArcSec::Evaluator* eval = NULL;
  Arc::ClassLoader* classloader = NULL;

  //Get the lib path from environment, and put it into the configuration xml node
  std::list<std::string> plugins = Arc::ArcLocation::GetPlugins();

  Arc::XMLNode node;
  std::list<Arc::XMLNode>::iterator it;
  bool found = false;
  for( it = class_config_list_.begin(); it != class_config_list_.end(); it++) {
    node = (*it);
    if((std::string)(node["PDPConfig"]["Evaluator"].Attribute("name")) == classname) { found = true; break; }
  }
  if(found) {
    bool has_covered = false;
    for(std::list<std::string>::iterator p = plugins.begin();p!=plugins.end();++p) {
      for(int i=0;;i++) {
        Arc::XMLNode cn = node["ModuleManager"]["Path"][i];
        if(!cn) break;
        if((std::string)(cn) == (*p)){ has_covered = true; break; }
      }
      if(!has_covered)
        node["ModuleManager"].NewChild("Path")=*p;
    }
 
    Arc::Config modulecfg(node);
    classloader = Arc::ClassLoader::getClassLoader(&modulecfg);
    //Dynamically load Evaluator object according to configure information. It should be the caller to free the object
    eval = dynamic_cast<Evaluator*>(classloader->Instance(classname, (void**)(void*)&node));
  }
  
  return eval;
}

Request* EvaluatorLoader::getRequest(std::string& classname, std::string& reqstr) {
  Arc::XMLNode node(reqstr);
  return (getRequest(classname, &node));
}

Request* EvaluatorLoader::getRequest(std::string& classname, const char* reqfile) {
  std::string str, reqstr;
  std::ifstream f(reqfile);
  if(!f) logger.msg(Arc::ERROR,"Failed to read request file %s", reqfile);

  while (f >> str) {
    reqstr.append(str);
    reqstr.append(" ");
  }
  f.close();

  Arc::XMLNode node(reqstr);
  return (getRequest(classname, &node));
}

Request* EvaluatorLoader::getRequest(std::string& classname, Arc::XMLNode* reqnode) {
  ArcSec::Request* req = NULL;
  Arc::ClassLoader* classloader = NULL;

  //Get the lib path from environment, and put it into the configuration xml node
  std::list<std::string> plugins = Arc::ArcLocation::GetPlugins();

  Arc::XMLNode node;
  std::list<Arc::XMLNode>::iterator it;
  bool found = false;
  for( it = class_config_list_.begin(); it != class_config_list_.end(); it++) {
    node = (*it);
    if((std::string)(node["PDPConfig"]["Request"].Attribute("name")) == classname) { found = true; break; }
  }

  if(found) {
    bool has_covered = false;
    for(std::list<std::string>::iterator p = plugins.begin();p!=plugins.end();++p) {
      for(int i=0;;i++) {
        Arc::XMLNode cn = node["ModuleManager"]["Path"][i];
        if(!cn) break;
        if((std::string)(cn) == (*p)){ has_covered = true; break; }
      }
      if(!has_covered)
        node["ModuleManager"].NewChild("Path")=*p;
    }

    Arc::Config modulecfg(node);
    classloader = Arc::ClassLoader::getClassLoader(&modulecfg);
    //Dynamically load Request object according to configure information. It should be the caller to free the object
    req = dynamic_cast<Request*>(classloader->Instance(classname, (void**)(void*)reqnode));
  }

  return req;
}

Policy* EvaluatorLoader::getPolicy(std::string& classname, std::string& policystr) {
  Arc::XMLNode node(policystr);
  return (getPolicy(classname, &node));
}

Policy* EvaluatorLoader::getPolicy(std::string& classname, const char* policyfile) {
  std::string str, policystr;
  std::ifstream f(policyfile);
  if(!f) logger.msg(Arc::ERROR,"Failed to read request file %s", policyfile);

  while (f >> str) {
    policystr.append(str);
    policystr.append(" ");
  }
  f.close();

  Arc::XMLNode node(policystr);
  return (getPolicy(classname, &node));
}

Policy* EvaluatorLoader::getPolicy(std::string& classname, Arc::XMLNode* policynode) {
  ArcSec::Policy* policy = NULL;
  Arc::ClassLoader* classloader = NULL;

  //Get the lib path from environment, and put it into the configuration xml node
  std::list<std::string> plugins = Arc::ArcLocation::GetPlugins();

  Arc::XMLNode node;
  std::list<Arc::XMLNode>::iterator it;
  bool found = false;
  for( it = class_config_list_.begin(); it != class_config_list_.end(); it++) {
    node = (*it);
    if((std::string)(node["PDPConfig"]["Policy"].Attribute("name")) == classname) { found = true; break; }
  }

  if(found) {
    bool has_covered = false;
    for(std::list<std::string>::iterator p = plugins.begin();p!=plugins.end();++p) {
      for(int i=0;;i++) {
        Arc::XMLNode cn = node["ModuleManager"]["Path"][i];
        if(!cn) break;
        if((std::string)(cn) == (*p)){ has_covered = true; break; }
      }
      if(!has_covered)
        node["ModuleManager"].NewChild("Path")=*p;
    }

    Arc::Config modulecfg(node);
    classloader = Arc::ClassLoader::getClassLoader(&modulecfg);
    //Dynamically load Policy object according to configure information. It should be the caller to free the object
    policy = dynamic_cast<Policy*>(classloader->Instance(classname, (void**)(void*)policynode));
  }

  return policy;
}

}
