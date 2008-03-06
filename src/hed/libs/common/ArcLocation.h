#ifndef __ARC_ARCLOCATION_H__
#define __ARC_ARCLOCATION_H__

#include <string>

namespace Arc {

  class ArcLocation {
   public:
    static void Init(std::string path);
    static const std::string& Get();
   private:
    static std::string location;
  };

} // namespace Arc

#endif // __ARC_ARCLOCATION_H__
