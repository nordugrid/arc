// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ACCLOADER_H__
#define __ARC_ACCLOADER_H__

#include <string>
#include <map>

#include <arc/loader/Loader.h>
#include <arc/client/ACC.h>

namespace Arc {

  class ACCLoader
    : public Loader {
  public:
    typedef std::map<std::string, ACC*> acc_container_t;

  private:
    acc_container_t accs_;

    void make_elements(Config& cfg);

  public:
    ACCLoader() {}
    /** Constructor that takes whole XML configuration and creates
        component chains */
    ACCLoader(Config& cfg);
    /** Destructor destroys all components created by constructor */
    ~ACCLoader();
    /** Access entry ACCs.
        Those are components exposed for external access using 'entry'
        attribute */
    ACC* getACC(const std::string& id);
  };

} // namespace Arc

#endif /* __ARC_ACCLOADER_H__ */
