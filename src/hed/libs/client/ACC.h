#ifndef __ARC_ACC_H__
#define __ARC_ACC_H__

namespace Arc {

  class Config;

  class ACC {
   protected:
    ACC(Config*) {};
   public:
    virtual ~ACC() {};
  };

} // namespace Arc

#endif // __ARC_ACC_H__
