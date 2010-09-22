#ifndef GACL_H
#define GACL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxml/tree.h>
#include <gridsite.h>
#include <gridsite-gacl.h>

#ifdef __cplusplus
}
#endif

GRSTgaclAcl *NGACLloadAcl(char *filename);
GRSTgaclAcl *NGACLloadAclForFile(char *pathandfile);
GRSTgaclAcl *NGACLacquireAcl(const char *str);

#endif
