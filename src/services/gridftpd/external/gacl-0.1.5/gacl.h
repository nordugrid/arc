/*
   Copyright (c) 2002, Andrew McNab, University of Manchester
   All rights reserved.

   Redistribution and use in source and binary forms, with or
   without modification, are permitted provided that the following
   conditions are met:

     o Redistributions of source code must retain the above
       copyright notice, this list of conditions and the following
       disclaimer. 
     o Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
       disclaimer in the documentation and/or other materials
       provided with the distribution. 

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

/*------------------------------------------------------------------------*
 * For more information about GACL: http://www.gridpp.ac.uk/gacl/ *
 *------------------------------------------------------------------------*/

#ifndef GACL_LIB_VERSION
#define GACL_LIB_VERSION "x.x.x"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _GACLnamevalue { char                  *name;
                        char                  *value;
                        struct _GACLnamevalue *next;  } ;
 
struct _GACLcred { char                  *type;    
                   struct _GACLnamevalue *firstname;
                   struct _GACLcred      *next;      } ;

typedef struct _GACLcred   GACLcred; 

typedef int                GACLaction;
typedef unsigned int       GACLperm;

struct _GACLentry { GACLcred   *firstcred; 
                    GACLperm    allowed; 
                    GACLperm    denied;  
                    void       *next;	 } ;

typedef struct _GACLentry  GACLentry;

struct _GACLacl   { GACLentry *firstentry; } ;

typedef struct _GACLacl    GACLacl;  

struct _GACLuser  { GACLcred *firstcred;
                    /* a list of DN's, CAS, VOMS etc objects */ };
                    
typedef struct _GACLuser   GACLuser;

extern char      *gacl_perm_syms[];
extern GACLperm   gacl_perm_vals[];

#define GACL_PERM_NONE  0
#define GACL_PERM_READ  1
#define GACL_PERM_LIST  2
#define GACL_PERM_WRITE 4
#define GACL_PERM_ADMIN 8

#define GACLhasNone(perm)  (perm == 0)
#define GACLhasRead(perm)  ((perm & GACL_PERM_READ) != 0)
#define GACLhasList(perm)  ((perm & GACL_PERM_LIST) != 0)
#define GACLhasWrite(perm) ((perm & GACL_PERM_WRITE) != 0)
#define GACLhasAdmin(perm) ((perm & GACL_PERM_ADMIN) != 0)

#define GACL_ACTION_ALLOW 0
#define GACL_ACTION_DENY  1

#define GACL_ACL_FILE ".gacl"
#define GACL_DN_LISTS "/etc/grid-security/dn-lists"

int        GACLinit(void);

GACLcred  *GACLnewCred(char *);
int        GACLaddToCred(GACLcred *, char *, char *);
int        GACLfreeCred(GACLcred *);
int        GACLaddCred(GACLentry *, GACLcred *);
int        GACLdelCred(GACLentry *, GACLcred *);
int        GACLprintCred(GACLcred *, FILE *);

GACLentry *GACLnewEntry(void);
int        GACLfreeEntry(GACLentry *);
int        GACLaddEntry(GACLacl *, GACLentry *);
int        GACLprintEntry(GACLentry *, FILE *);

int        GACLprintPerm(GACLperm, FILE *);
int        GACLallowPerm(GACLentry *, GACLperm);
int        GACLunallowPerm(GACLentry *, GACLperm);
int        GACLdenyPerm(GACLentry *, GACLperm);
int        GACLundenyPerm(GACLentry *, GACLperm);
char      *GACLpermToChar(GACLperm);
GACLperm   GACLcharToPerm(char *);

GACLacl   *GACLnewAcl(void);
int        GACLfreeAcl(GACLacl *);
int        GACLprintAcl(GACLacl *, FILE *);
int        GACLsaveAcl(char *, GACLacl *);

GACLacl   *GACLloadAcl(char *);
GACLacl   *GACLloadAclForFile(char *);
int        GACLisAclFile(char *);

GACLuser *GACLnewUser(GACLcred *);
int       GACLfreeUser(GACLuser *);
int       GACLuserAddCred(GACLuser *, GACLcred *);
int       GACLuserHasCred(GACLuser *, GACLcred *);
GACLcred *GACLuserFindCredType(GACLuser *, char *);

int        GACLtestDnList(char *, GACLuser *);
GACLperm   GACLtestUserAcl(GACLacl *, GACLuser *);
GACLperm   GACLtestExclAcl(GACLacl *, GACLuser *);

char      *GACLurlEncode(char *);
char      *GACLmildUrlEncode(char *);

#ifdef __cplusplus
}
#endif /* __cplusplus */
