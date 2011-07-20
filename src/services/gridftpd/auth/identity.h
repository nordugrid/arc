#ifndef __ARC_IDENTITY_H__
#define __ARC_IDENTITY_H__

#include <string>
#include <list>

class Identity {
 public:
  class Item {
   protected:
    std::string type_;
    static std::string empty_;
   public:
    Item(void);
    virtual ~Item(void);
    const std::string& type(void) const { return type_; };
    virtual Item* duplicate(void) const;
    virtual const std::string& name(unsigned int n);
    virtual const std::string& value(unsigned int n);
    virtual const std::string& value(const char* name,unsigned int n);
    virtual std::string str(void);
  };
 protected:
  std::list<Item*> items_;
  static Item empty_;
 public:
  Identity(void);
  Identity(const Identity&);
  virtual ~Identity(void);
  Item* add(const Item* t);
  Item* use(Item* t);
  Item* operator[](unsigned int n);
  virtual Identity* duplicate(void) const;
  virtual bool operator==(Identity& id);
};

#endif // __ARC_IDENTITY_H__
