#ifndef __ARC_Service_H__
#define __ARC_Service_H__

#include "common/ArcConfig.h"

namespace Arc {

/** Service - last plugin in a Message Chain.
  This is virtual class which defines interface (in a future also 
 common functionality) for every Service plugin. Interface is made of 
 method process() which is called by Plexer class.
 */
class Service
{
    public:
        /** Example contructor - Server takes at least it's configuration subtree */
        Service(Arc::Config *cfg) { };
        virtual ~Service(void) { };
        /** Method for processing of information.
           This method is called by Plexer class.
           This method must process passed information and return valid 
          response. Respons may contain failure but still must tbe valid. */
        virtual void process(void) { };
};

} // namespace Arc

/** This structure describes Service stored in shared library.
   Each Service is detected by presence of element named 'descriptor' 
  of this type. It contains version number and pointer to function which 
  creates an instance of object inherited from Service class. */
typedef struct {
    int version;
    Arc::Service *(*get_instance)(Arc::Config *cfg);
} service_descriptor;

#endif /* __ARC_Service_H__ */
