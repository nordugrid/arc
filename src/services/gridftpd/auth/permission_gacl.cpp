#include "permission_gacl.h"


PermissionGACL::PermissionGACL(void) {
}

PermissionGACL::PermissionGACL(const Permission& p):Permission(p) {
}

PermissionGACL::~PermissionGACL(void) {
}

Permission* PermissionGACL::duplicate(void) const {
  return new PermissionGACL(*this);
}

bool PermissionGACL::hasRead(void) {
  if(get(Permission::object,Permission::read,Permission::allow)) return true;
  return false;
}

bool PermissionGACL::hasList(void) {
  if(get(Permission::object,Permission::info,Permission::allow)) return true;
  return false;
}

bool PermissionGACL::hasWrite(void) {
  if(get(Permission::object,Permission::write,Permission::allow)) return true;
  if(get(Permission::object,Permission::extend,Permission::allow)) return true;
  if(get(Permission::object,Permission::reduce,Permission::allow)) return true;
  return false;
}

bool PermissionGACL::hasAdmin(void) {
  if(get(Permission::permissions,Permission::write,Permission::allow)) return true;
  return false;
}

GACLperm PermissionGACL::has(void) {
  GACLperm p = GACL_PERM_NONE;
  if(hasAdmin()) p|=GACL_PERM_ADMIN;
  if(hasRead()) p|=GACL_PERM_READ;
  if(hasWrite()) p|=GACL_PERM_WRITE;
  if(hasList()) p|=GACL_PERM_LIST;
  return p;
}

GACLperm PermissionGACL::allowed(void) {
  GACLperm p = GACL_PERM_NONE;
  if(get(Permission::permissions,Permission::write,Permission::allow)) p|=GACL_PERM_LIST;
  if(get(Permission::object,Permission::write,Permission::allow)) p|=GACL_PERM_WRITE;
  if(get(Permission::object,Permission::extend,Permission::allow)) p|=GACL_PERM_WRITE;
  if(get(Permission::object,Permission::reduce,Permission::allow)) p|=GACL_PERM_WRITE;
  if(get(Permission::object,Permission::info,Permission::allow)) p|=GACL_PERM_LIST;
  if(get(Permission::object,Permission::read,Permission::allow)) p|=GACL_PERM_READ;
  return p;
}

GACLperm PermissionGACL::denied(void) {
  GACLperm p = GACL_PERM_NONE;
  if(get(Permission::permissions,Permission::write,Permission::deny)) p|=GACL_PERM_LIST;
  if(get(Permission::object,Permission::write,Permission::deny)) p|=GACL_PERM_WRITE;
  if(get(Permission::object,Permission::extend,Permission::deny)) p|=GACL_PERM_WRITE;
  if(get(Permission::object,Permission::reduce,Permission::deny)) p|=GACL_PERM_WRITE;
  if(get(Permission::object,Permission::info,Permission::deny)) p|=GACL_PERM_LIST;
  if(get(Permission::object,Permission::read,Permission::deny)) p|=GACL_PERM_READ;
  return p;
}

void PermissionGACL::allow(Permission::Object o,Permission::Action a) {
  set(o,a,Permission::allow);
}

void PermissionGACL::unallow(Permission::Object o,Permission::Action a) {
  if(get(o,a,Permission::allow)) set(o,a,Permission::undefined);
}

void PermissionGACL::deny(Permission::Object o,Permission::Action a) {
  set(o,a,Permission::deny);
}

void PermissionGACL::undeny(Permission::Object o,Permission::Action a) {
  if(get(o,a,Permission::deny)) set(o,a,Permission::undefined);
}

bool PermissionGACL::allow(GACLperm p) {
  if(GACL_PERM_READ & p) {
    allow(Permission::object,Permission::read);
    allow(Permission::metadata,Permission::read);
  };
  if(GACL_PERM_WRITE & p) {
    allow(Permission::object,Permission::create);
    allow(Permission::metadata,Permission::create);
    allow(Permission::object,Permission::write);
    allow(Permission::metadata,Permission::write);
    allow(Permission::object,Permission::extend);
    allow(Permission::metadata,Permission::extend);
    allow(Permission::object,Permission::reduce);
    allow(Permission::metadata,Permission::reduce);
    allow(Permission::object,Permission::remove);
    allow(Permission::metadata,Permission::remove);
  };
  if(GACL_PERM_LIST & p) {
    allow(Permission::object,Permission::info);
    allow(Permission::metadata,Permission::info);
  };
  if(GACL_PERM_ADMIN & p) {
    allow(Permission::permissions,Permission::read);
    allow(Permission::permissions,Permission::create);
    allow(Permission::permissions,Permission::write);
    allow(Permission::permissions,Permission::extend);
    allow(Permission::permissions,Permission::reduce);
    allow(Permission::permissions,Permission::remove);
    allow(Permission::permissions,Permission::info);
  };
  return true;
}

bool PermissionGACL::unallow(GACLperm p) {
  if(GACL_PERM_READ & p) {
    unallow(Permission::object,Permission::read);
    unallow(Permission::metadata,Permission::read);
  };
  if(GACL_PERM_WRITE & p) {
    unallow(Permission::object,Permission::create);
    unallow(Permission::metadata,Permission::create);
    unallow(Permission::object,Permission::write);
    unallow(Permission::metadata,Permission::write);
    unallow(Permission::object,Permission::extend);
    unallow(Permission::metadata,Permission::extend);
    unallow(Permission::object,Permission::reduce);
    unallow(Permission::metadata,Permission::reduce);
    unallow(Permission::object,Permission::remove);
    unallow(Permission::metadata,Permission::remove);
  };
  if(GACL_PERM_LIST & p) {
    unallow(Permission::object,Permission::info);
    unallow(Permission::metadata,Permission::info);
  };
  if(GACL_PERM_ADMIN & p) {
    unallow(Permission::permissions,Permission::read);
    unallow(Permission::permissions,Permission::create);
    unallow(Permission::permissions,Permission::write);
    unallow(Permission::permissions,Permission::extend);
    unallow(Permission::permissions,Permission::reduce);
    unallow(Permission::permissions,Permission::remove);
    unallow(Permission::permissions,Permission::info);
  };
  return true;
}

bool PermissionGACL::deny(GACLperm p) {
  if(GACL_PERM_READ & p) {
    deny(object,read);   deny(metadata,read);
  };
  if(GACL_PERM_WRITE & p) {
    deny(object,create); deny(metadata,create);
    deny(object,write);  deny(metadata,write);
    deny(object,extend); deny(metadata,extend);
    deny(object,reduce); deny(metadata,reduce);
    deny(object,remove); deny(metadata,remove);
  };
  if(GACL_PERM_LIST & p) {
    deny(object,info);   deny(metadata,info);
  };
  if(GACL_PERM_ADMIN & p) {
    deny(permissions,read);
    deny(permissions,create);
    deny(permissions,write);
    deny(permissions,extend);
    deny(permissions,reduce);
    deny(permissions,remove);
    deny(permissions,info);
  };
  return true;
}

bool PermissionGACL::undeny(GACLperm p) {
  if(GACL_PERM_READ & p) {
    undeny(object,read);   undeny(metadata,read);
  };
  if(GACL_PERM_WRITE & p) {
    undeny(object,create); undeny(metadata,create);
    undeny(object,write);  undeny(metadata,write);
    undeny(object,extend); undeny(metadata,extend);
    undeny(object,reduce); undeny(metadata,reduce);
    undeny(object,remove); undeny(metadata,remove);
  };
  if(GACL_PERM_LIST & p) {
    undeny(object,info);   undeny(metadata,info);
  };
  if(GACL_PERM_ADMIN & p) {
    undeny(permissions,read);
    undeny(permissions,create);
    undeny(permissions,write);
    undeny(permissions,extend);
    undeny(permissions,reduce);
    undeny(permissions,remove);
    undeny(permissions,info);
  };
  return true;
}

