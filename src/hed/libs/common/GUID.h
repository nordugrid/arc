#ifndef __ARC_GUID_H__
#define __ARC_GUID_H__

#include <string>

namespace Arc {

  /// This function generates a random identifier which is quite unique as well
  void GUID(std::string& guid);
  /// This function generates uuid
  std::string UUID(void);

} // namespace Arc

#endif // __ARC_GUID_H__
