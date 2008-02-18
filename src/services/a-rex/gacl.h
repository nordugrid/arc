#include <arc/XMLNode.h>

#define GACL_PERM_NONE  0
#define GACL_PERM_READ  1
#define GACL_PERM_LIST  2
#define GACL_PERM_WRITE 4
#define GACL_PERM_ADMIN 8
#define GACL_PERM_ALL   15

int GACLEvaluate(Arc::XMLNode gacl,Arc::XMLNode subject);

