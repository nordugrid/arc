#include "DBranch.h"

namespace Arc {

DBranch::DBranch(void):item_(new DItem) {
}

DBranch::~DBranch(void) {
  delete item_;
}

DBranch& DBranch::operator[](const std::string& name) {
  for(std::list<DBranch*>::iterator b = branches_.begin();b!=branches_.end();++b) {
    if((*b)->name_ == name) return **b;
  };
  return **(branches_.insert(branches_.end(),new DBranch));
}

DBranch& DBranch::operator[](int pos) {
  std::list<DBranch*>::iterator b = branches_.begin();
  for(;b!=branches_.end();++b) {
    if(pos <= 0) break; --pos;
  };
  if(b != branches_.end()) return **b;
  return **(branches_.insert(b,new DBranch));
}

DBranch& DBranch::Add(const std::string&,int pos) {
  std::list<DBranch*>::iterator b = branches_.begin();
  for(;b!=branches_.end();++b) {
    if(pos <= 0) break; --pos;
  };
  return **(branches_.insert(b,new DBranch));
}

int DBranch::Size(void) {
  return branches_.size();
}

const std::string& DBranch::Name(void) {
  return name_;
}

DItem& DBranch::Item(void) {
  return *item_;
}

DBranch::operator DItem&(void) {
  return *item_;
}

DBranch& DBranch::operator=(const DItem& item) {
  DItem* old_item = item_;
  DItem* new_item = item.New();
  if(new_item) {
    item_=new_item;
    if(old_item) delete old_item;
  };
  return *this;
}

void DBranch::Assign(DItem* item) {
  DItem* old_item = item_;
  if(!item) {
    item_=new DItem;
  } else {
    item_=item;
  };
  if(old_item) delete old_item;
}

DItemString::DItemString(const std::string& s):value_(s) {
}

DItemString::DItemString(const char* s):value_(s) {
}

DItem* DItemString::New(void) const {
  return new DItemString(value_);
}

DItemString::~DItemString(void) {
}

std::string DItemString::str() const {
  return value_;
}

bool DItemString::operator==(const DItem& item) {
  const DItemString* sitem = dynamic_cast<const DItemString*>(&item);
  if(sitem) return (value_ == sitem->value_);
  return (value_ == item.str());
}

bool DItemString::operator<(const DItem& item) {
  const DItemString* sitem = dynamic_cast<const DItemString*>(&item);
  if(sitem) return (value_ < sitem->value_);
  return (value_ < item.str());
}

bool DItemString::operator>(const DItem& item) {
  const DItemString* sitem = dynamic_cast<const DItemString*>(&item);
  if(sitem) return (value_ < sitem->value_);
  return (value_ < item.str());
}

bool DItemString::operator<=(const DItem& item) {
  const DItemString* sitem = dynamic_cast<const DItemString*>(&item);
  if(sitem) return (value_ <= sitem->value_);
  return (value_ <= item.str());
}

bool DItemString::operator>=(const DItem& item) {
  const DItemString* sitem = dynamic_cast<const DItemString*>(&item);
  if(sitem) return (value_ >= sitem->value_);
  return (value_ >= item.str());
}

DItemString::operator bool(void) {
  return !value_.empty();
}

bool DItemString::operator!(void) {
  return value_.empty();
}

DItemString& DItemString::operator=(const DItem& item) {
  const DItemString* sitem = dynamic_cast<const DItemString*>(&item);
  if(sitem) {
    value_=sitem->value_;
  } else {
    value_=item.str();
  };
  return *this;
}

DItemString& DItemString::operator=(const std::string& s) {
  value_=s;
  return *this;
}


}
