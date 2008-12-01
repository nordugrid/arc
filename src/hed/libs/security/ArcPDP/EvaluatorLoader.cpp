#ifdef HAVE_CONFIG_H
#include <config.h> 
#endif

#include <iostream>
#include <glibmm.h>
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/security/ClassLoader.h>
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

Evaluator* EvaluatorLoader::getEvaluator(const std::string& classname) {
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
  } else {
    // Loading unknown evaluator
    Arc::XMLNode cfg("\
      <ArcConfig\
       xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\">\
       <ModuleManager/>\
      </ArcConfig>");
    for(std::list<std::string>::iterator plugin = plugins.begin();plugin!=plugins.end();++plugin) {
      cfg["ModuleManager"].NewChild("Path")=*plugin;
      try {
        Glib::Dir dir(*plugin);
        for(Glib::DirIterator file = dir.begin(); file != dir.end(); file++) {
          if((*file).substr(0, 3) == "lib") {
            std::string name = (*file).substr(3,(*file).find('.')-3);
            Arc::XMLNode plugcfg = cfg.NewChild("Plugins");
            plugcfg.NewAttribute("Name")=name;
            plugcfg.NewChild("Plugin").NewAttribute("Name")="__arc_evaluator_modules__";
          };
        };
      } catch (Glib::FileError) {};
    }
    cfg.New(node);
  }
  Arc::Config modulecfg(node);
  classloader = Arc::ClassLoader::getClassLoader(&modulecfg);
  //Dynamically load Evaluator object according to configure information. 
  //It should be the caller to free the object
  eval = (Evaluator*)(classloader->Instance(classname, &node, "__arc_evaluator_modules__"));

  if(!eval) logger.msg(Arc::ERROR, "Can not load arc evaluator object: %s",classname); 
  return eval;
}

Evaluator* EvaluatorLoader::getEvaluator(const Policy* policy) {
  if(!policy) return NULL;
  return getEvaluator(policy->getEvalName());
}

Evaluator* EvaluatorLoader::getEvaluator(const Request* request) {
  if(!request) return NULL;
  return getEvaluator(request->getEvalName());
}

Request* EvaluatorLoader::getRequest(const std::string& classname, const Source& requestsource) {
  ArcSec::Request* req = NULL;
  Arc::ClassLoader* classloader = NULL;
  
  //Get the request node
  Arc::XMLNode reqnode = requestsource.Get();

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
    //Dynamically load Request object according to configure information. 
    //It should be the caller to free the object
    req = (Request*)(classloader->Instance(classname, &reqnode, "__arc_request_modules__"));
  }
  
  if(!req) logger.msg(Arc::ERROR, "Can not load arc request object: %s",classname); 
  return req;
}

Policy* EvaluatorLoader::getPolicy(const std::string& classname, const Source& policysource) {
  ArcSec::Policy* policy = NULL;
  Arc::ClassLoader* classloader = NULL;

  //Get policy node
  Arc::XMLNode policynode = policysource.Get();

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
    //Dynamically load Policy object according to configure information. 
    //It should be the caller to free the object
    policy = (Policy*)(classloader->Instance(classname, &policynode, "__arc_policy_modules__"));
  }

  if(!policy) logger.msg(Arc::ERROR, "Can not load policy object: %s",classname); 
  return policy;
}

Policy* EvaluatorLoader::getPolicy(const Source& policysource) {
  ArcSec::Policy* policy = NULL;
  Arc::ClassLoader* classloader = NULL;

  //Get policy node
  Arc::XMLNode policynode = policysource.Get();

  //Get the lib path from environment, and put it into the configuration xml node
  std::list<std::string> plugins = Arc::ArcLocation::GetPlugins();

  Arc::XMLNode cfg("\
    <ArcConfig\
     xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\">\
     <ModuleManager/>\
    </ArcConfig>");
  for(std::list<std::string>::iterator plugin = plugins.begin();plugin!=plugins.end();++plugin) {
    cfg["ModuleManager"].NewChild("Path")=*plugin;
    try {
      Glib::Dir dir(*plugin);
      for(Glib::DirIterator file = dir.begin(); file != dir.end(); file++) {
        if((*file).substr(0, 3) == "lib") {
          std::string name = (*file).substr(3,(*file).find('.')-3);
          Arc::XMLNode plugcfg = cfg.NewChild("Plugins");
          plugcfg.NewAttribute("Name")=name;
          plugcfg.NewChild("Plugin").NewAttribute("Name")="__arc_policy_modules__";
          // ?? plugcfg["Plugin"]="policy"; ??
        };
      };
    } catch (Glib::FileError) {};
  };

  Arc::Config modulecfg(cfg);
  classloader = Arc::ClassLoader::getClassLoader(&modulecfg);
  //Dynamically load Policy object according to configure information. 
  //It should be the caller to free the object
  policy = (Policy*)(classloader->Instance(&policynode, "__arc_policy_modules__"));

  if(!policy) logger.msg(Arc::ERROR, "Can not load policy object"); 
  return policy;
}

Request* EvaluatorLoader::getRequest(const Source& requestsource) {
  ArcSec::Request* request = NULL;
  Arc::ClassLoader* classloader = NULL;

  //Get policy node
  Arc::XMLNode requestnode = requestsource.Get();

  //Get the lib path from environment, and put it into the configuration xml node
  std::list<std::string> plugins = Arc::ArcLocation::GetPlugins();

  Arc::XMLNode cfg("\
    <ArcConfig\
     xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\">\
     <ModuleManager/>\
    </ArcConfig>");
  for(std::list<std::string>::iterator plugin = plugins.begin();plugin!=plugins.end();++plugin) {
    cfg["ModuleManager"].NewChild("Path")=*plugin;
    try {
      Glib::Dir dir(*plugin);
      for(Glib::DirIterator file = dir.begin(); file != dir.end(); file++) {
        if((*file).substr(0, 3) == "lib") {
          std::string name = (*file).substr(3,(*file).find('.')-3);
          Arc::XMLNode plugcfg = cfg.NewChild("Plugins");
          plugcfg.NewAttribute("Name")=name;
          plugcfg.NewChild("Plugin").NewAttribute("Name")="__arc_request_modules__";
          // ?? plugcfg["Plugin"]="request"; ??
        };
      };
    } catch (Glib::FileError) {};
  };

  Arc::Config modulecfg(cfg);
  classloader = Arc::ClassLoader::getClassLoader(&modulecfg);
  //Dynamically load Request object according to configure information. 
  //It should be the caller to free the object
  request = (Request*)(classloader->Instance(&requestnode, "__arc_request_modules__"));

  if(!request) logger.msg(Arc::ERROR, "Can not load request object"); 
  return request;
}

}
