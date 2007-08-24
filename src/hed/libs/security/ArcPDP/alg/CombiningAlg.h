#ifndef __ARC_COMBININGALG_H__
#define __ARC_COMBININGALG_H__

namespace Arc {

class CombiningAlg {
protected:
 std::string algId;

public:
  CombiningAlg(){};
  virtual ~CombiningAlg(){};

public:
  virtual Result combine(Evaluation* ctx, std::list<Policy*> policies){};
};

} // namespace Arc

#endif /* __ARC_COMBININGALG_H__ */

