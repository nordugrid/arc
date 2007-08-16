#ifndef __ARC_COMBININGALG_H__
#define __ARC_COMBININGALG_H__

namespace Arc {

class CombiningAlg {
public:
  CombiningAlg();
  virtual ~CombiningAlg();

public:
  virtual boolean evaluate(){};
};

} // namespace Arc

#endif /* __ARC_COMBININGALG_H__ */

