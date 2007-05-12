#ifndef __ARC_CONFIG_H__
#define __ARC_CONFIG_H__

#include <string>
#include <list>
#include "XMLNode.h"

namespace Arc {

/** Configuration element - represents (sub)tree of ARC configuration.
   This class is intended to be used to pass configuration details to 
  various parts of HED and external modules. Currently it's just a 
  wrapper over XML tree. But than may change in a future, although
  interface should be preserved. 
   Currently it is capable of loading XML configuration document from
  file. In future it will be capable of loading more user-readable 
  format and process it into tree-like structure convenient for 
  machine processing (XML-like).
   So far there are no schema and/or namespaces assigned.
 */
class Config: public XMLNode {
    public:
	/** Dummy constructor - produces empty structure */
        Config() { };
	/** Loads configuration document from file 'filename' */
        Config(const char *filename);
	/** Parse configuration document from memory */
        Config(const std::string &xml_str): XMLNode(xml_str) { };
	/** Acquire existing XML (sub)tree.
	  Content is not copied. Make sure XML tree is not destroyed
	  while in use by this object. */
        Config(Arc::XMLNode xml): XMLNode(xml) { };
        ~Config(void);
	/** Print structure of document.
	  For debuging purposes. Printed content is not an XML document. */
        void print(void);
};

}; // namespace Arc 

#endif /* __ARC_CONFIG_H__ */
