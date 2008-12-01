#ifndef __ARC_DMCLOADER_H__
#define __ARC_DMCLOADER_H__

#include <string>
#include <map>

#include <arc/loader/Loader.h>

namespace Arc {

  class Config;
  class Logger;
  class DMC;

  class DMCLoader: public Loader {
   public:
    typedef std::map<std::string, DMC*>        dmc_container_t;

   private:
    /** Set of labeled DMC objects */
    dmc_container_t dmcs_;

    void make_elements(Config& cfg);

   public:
    DMCLoader() {};
    /** Constructor that takes whole XML configuration and creates
       component chains */
    DMCLoader(Config& cfg);
    /** Destructor destroys all components created by constructor */
    ~DMCLoader();
    DMC* getDMC(const std::string& id);
 };

} // namespace Arc

#endif /* __ARC_DMCLOADER_H__ */
