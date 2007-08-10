#ifndef __ARC_FUNCTION_H__
#define __ARC_FUNCTION_H__

namespace Arc {

class Function {
public:
  Function();
  virtual ~Function();

public:
  virtual boolean evaluate(){};
};

} // namespace Arc

#endif /* __ARC_FUNCTION_H__ */

