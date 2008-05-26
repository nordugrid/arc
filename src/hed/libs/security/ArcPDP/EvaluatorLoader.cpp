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
     </Plugins>\
     <pdp:PDPConfig>\
          <pdp:AttributeFactory name='attr.factory' />\
          <pdp:CombingAlgorithmFactory name='alg.factory' />\
          <pdp:FunctionFactory name='fn.factory' />\
          <pdp:Evaluator name='arc.evaluator' />\
          <pdp:Request name='arc.request' />\
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
    for(std::list<std::string>::iterator p = plugins.begin();p!=plugins.end();++p)
      node["ModuleManager"].NewChild("Path")=*p;
    
    Arc::Config modulecfg(node);
    classloader = Arc::ClassLoader::getClassLoader(&modulecfg);

    //Dynamically load Evaluator object according to configure information. It should be the caller to free the evaluator object
    eval = dynamic_cast<Evaluator*>(classloader->Instance(classname, (void**)(void*)&node));
  }
  
  return eval;
}

}
