#include "identity_voms.h"

std::string IdentityItemVOMS::vo_name_("vo");
std::string IdentityItemVOMS::voms_name_("voms");
std::string IdentityItemVOMS::group_name_("group");
std::string IdentityItemVOMS::role_name_("role");
std::string IdentityItemVOMS::cap_name_("capability");

IdentityItemVOMS::IdentityItemVOMS(const IdentityItemVOMS& v) {
  vo_=v.vo_;
  voms_=v.voms_;
  group_=v.group_;
  role_=v.role_;
  cap_=v.cap_;
}

IdentityItemVOMS::~IdentityItemVOMS(void) { }

Identity::Item* IdentityItemVOMS::duplicate(void) const {
  return new IdentityItemVOMS(*this);
}

const std::string& IdentityItemVOMS::name(unsigned int n) {
  switch(n) {
    case 0: return vo_name_;
    case 1: return voms_name_;
    case 2: return group_name_;
    case 3: return role_name_;
    case 4: return cap_name_;
  };
  return empty_;
}

const std::string& IdentityItemVOMS::value(unsigned int n) {
  switch(n) {
    case 0: return vo_;
    case 1: return voms_;
    case 2: return group_;
    case 3: return role_;
    case 4: return cap_;
  };
  return empty_;
}

const std::string& IdentityItemVOMS::value(const char* name,unsigned int n) {
  if(vo_name_ == name) return vo_;
  if(voms_name_ == name) return voms_;
  if(group_name_ == name) return group_;
  if(role_name_ == name) return role_;
  if(cap_name_ == name) return cap_;
  return empty_;
}

IdentityItemVOMS::IdentityItemVOMS(const char* vo,const char* voms,const char* group,const char* role,const char* cap) {
  vo_=vo;
  voms_=voms;
  group_=group;
  role_=role;
  cap_=cap;
}

