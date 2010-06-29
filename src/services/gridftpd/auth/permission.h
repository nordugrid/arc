#ifndef __ARC_PERMISSION_H__
#define __ARC_PERMISSION_H__

class Permission {
 public:
  typedef enum {
    object = 0,
    metadata = 1,
    permissions = 2
  } Object;
  typedef enum {
    create = 0,
    read = 1,
    write = 2,
    extend = 3,
    reduce = 4,
    remove = 5,
    info = 6
  } Action;
  typedef enum {
    undefined = 0,
    allow = 1,
    deny = 2
  } Perm;
 private:
  Perm perms_[3][7];
 public:
  Permission(void);
  Permission(const Permission& p);
  virtual ~Permission(void);
  bool set(Object o,Action a,Perm p);
  bool set_conditional(Object o,Action a,Perm p);
  bool get(Object o,Action a,Perm p);
  bool get_conditional(Object o,Action a,Perm p);
  virtual Permission* duplicate(void) const;
};

#endif // __ARC_PERMISSION_H__
