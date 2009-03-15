// -*- indent-tabs-mode: nil -*-

#include <string>
#include <list>

namespace Arc {

  class DItem {
  public:
    DItem(void) {}
    virtual ~DItem(void) {}
    virtual DItem* New(void) const {
      return new DItem;
    }
    virtual std::string str() const {
      return "";
    }
    virtual bool operator==(const DItem&) {
      return false;
    }
    virtual bool operator<(const DItem&) {
      return false;
    }
    virtual bool operator>(const DItem&) {
      return false;
    }
    virtual bool operator<=(const DItem&) {
      return false;
    }
    virtual bool operator>=(const DItem&) {
      return false;
    }
    virtual operator bool(void) {
      return false;
    }
    virtual bool operator!(void) {
      return true;
    }
    virtual DItem& operator=(const DItem&) {
      return *this;
    }
    virtual DItem& operator=(const std::string&) {
      return *this;
    }
  };

  class DBranch {
  protected:
    std::string name_;
    std::list<DBranch*> branches_;
    DItem *item_;
  public:
    DBranch(void);
    ~DBranch(void);

    DBranch& operator[](const std::string&);
    DBranch& operator[](int);
    DBranch& Add(const std::string&, int pos = -1);
    int Size(void);
    const std::string& Name(void);

    DItem& Item(void);
    operator DItem&(void);
    DBranch& operator=(const DItem&);
    void Assign(DItem*);
  };

  class DItemString
    : public DItem {
  protected:
    std::string value_;
  public:
    DItemString(void);
    DItemString(const std::string&);
    DItemString(const char*);
    virtual ~DItemString(void);
    virtual DItem* New(void) const;
    virtual std::string str() const;
    virtual bool operator==(const DItem&);
    virtual bool operator<(const DItem&);
    virtual bool operator>(const DItem&);
    virtual bool operator<=(const DItem&);
    virtual bool operator>=(const DItem&);
    virtual operator bool(void);
    virtual bool operator!(void);
    virtual DItemString& operator=(const DItem&);
    virtual DItemString& operator=(const std::string&);
  };

}
