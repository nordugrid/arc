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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>              
#include <string.h>

#ifndef HAVE_OLD_LIBXML_INCLUDES
#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#else
#include <xmlmemory.h>
#include <tree.h>
#include <parser.h>
#ifndef LIBXML_TEST_VERSION
#define LIBXML_TEST_VERSION
#endif
#ifndef xmlChildrenNode
#define xmlChildrenNode childs
#endif
#endif

#include "gacl.h"

/*                                                                      *
 * Global variables, shared by all GACL functions by private to libgacl *
 *                                                                      */
 
// char *gacl_group_dir = NULL;

char     *gacl_perm_syms[] =  { "none",
                                "read",
                                "list",
                                "write",
                                "admin",
                                NULL              };

GACLperm gacl_perm_vals[] =  {   GACL_PERM_NONE,
                                 GACL_PERM_READ,
                                 GACL_PERM_LIST,
                                 GACL_PERM_WRITE,
                                 GACL_PERM_ADMIN,
                                 -1                };
                                 
int GACLinit(void)
{
  xmlInitParser();

  LIBXML_TEST_VERSION

  xmlKeepBlanksDefault(0);

  return 1;
}                             

/*                                             *
 * Functions to manipulate GACLcred structures *
 *                                             */

GACLcred *GACLnewCred(char *type)
/*
    GACLnewCred - allocate a new GACLcred structure, and return
                  it's pointer or NULL on (malloc) error.
*/
{
  GACLcred *newcred; 

  if (type == NULL) return NULL;

  newcred = malloc(sizeof(GACLcred));
  if (newcred == NULL) return NULL;
  
  newcred->type      = strdup(type);
  newcred->firstname = NULL;
  newcred->next      = NULL;

  return newcred;
}

int GACLaddToCred(GACLcred *cred, char *name, char *value)
/*
    GACLaddToCred - add a name/value pair to a GACLcred
*/
{
  struct _GACLnamevalue *p;
  
  if (cred->firstname == NULL) 
    {
      cred->firstname = malloc(sizeof (struct _GACLnamevalue));
      (cred->firstname)->name  = name?strdup(name):NULL;
      (cred->firstname)->value = value?strdup(value):NULL;
      (cred->firstname)->next  = NULL;
      return 1;
    }

  p = cred->firstname; 
  
  while (p->next != NULL) p = p->next;
  
   p->next = malloc(sizeof(struct _GACLnamevalue));
  (p->next)->name  = name?strdup(name):"";
  (p->next)->value = value?strdup(value):"";
  (p->next)->next  = NULL;
  
  return 1;
}

int GACLfreeNameValue(struct _GACLnamevalue *p)
{
  if (p == NULL) return 1;
  
  if (p->next != NULL) GACLfreeNameValue(p->next);
  
  free(p->name);
  free(p->value);
  free(p);
  
  return 1;
}

int GACLfreeCred(GACLcred *cred)
/*
    GACLfreeCred - free memory structures of a GACLcred, returning 1 always!
*/
{
  if (cred == NULL) return 1;

  GACLfreeNameValue(cred->firstname);  
  free(cred->type);
  free(cred);
  
  return 1;
}

int GACLfreeCreds(GACLcred *firstcred)
/*
    GACLfreeCreds - free a cred and all the creds in its *next chain
*/
{
  if (firstcred == NULL) return 0;
  
  if (firstcred->next != NULL) GACLfreeCreds(firstcred->next);
  
  return GACLfreeCred(firstcred);
}

int GACLinsertCred(GACLcred *firstcred, GACLcred *newcred)
/* 
    (Private)
    
    GACLinsertCred - insert a cred in the *next chain of firstcred

    FOR THE MOMENT THIS JUST APPENDS
*/
{
  if (firstcred == NULL) return 0;
  
  if (firstcred->next == NULL)
    {
      firstcred->next = newcred;
      return 1;
    }

  return GACLinsertCred(firstcred->next, newcred);     
}

int GACLaddCred(GACLentry *entry, GACLcred *cred)
/*  
    GACLaddCred - add a new credential to an existing entry, returning 1
    on success or 0 on error 
*/ 
{
  if (entry == NULL) return 0;
 
  if (entry->firstcred == NULL) 
    {
      entry->firstcred = cred;
      return 1;
    }
  else return GACLinsertCred(entry->firstcred, cred);
}

int GACLremoveCred(GACLcred *firstcred, GACLcred *oldcred)
/* 
    (Private)

    GACLremoveCred - remove a cred in the *next chain of firstcred
                     and relink the chain
*/
{
  if (firstcred == NULL) return 0;

// yeah, I know
}

int GACLdelCred(GACLentry *entry, GACLcred *cred)
/*  
    GACLdelCred - remove a new cred from an entry, returning 1
    on success (or absense) or 0 on error.
*/ 
{
  if (entry == NULL) return 0;

  return GACLremoveCred(entry->firstcred, cred);
}

int GACLprintCred(GACLcred *cred, FILE *fp)
/* 
   GACLprintCred - print a credential and any name-value pairs is contains
*/
{
  struct _GACLnamevalue *p;

  if (cred->firstname != NULL)
    {
      fprintf(fp, "<%s>\n", cred->type);
    
      p = cred->firstname;
      
      do { 
           fprintf(fp, "<%s>%s</%s>\n", p->name, p->value?p->value:"", p->name);
           p = p->next;
         
         } while (p != NULL);

      fprintf(fp, "</%s>\n", cred->type);
    }
  else fprintf(fp, "<%s/>\n", cred->type);
  
  return 1;  
}

/*                                              *
 * Functions to manipulate GACLentry structures *
 *                                              */

GACLentry *GACLnewEntry(void)
/*
    GACLnewEntry - allocate space for a new entry, returning its pointer
                   or NULL on failure.
*/
{
  GACLentry *newentry;
  
  newentry = (GACLentry *) malloc(sizeof(GACLentry));
  if (newentry == NULL) return NULL;

  newentry->firstcred    = NULL;
  newentry->allowed      = 0;
  newentry->denied       = 0;
  newentry->next         = NULL;
      
  return newentry;
}

int GACLfreeEntry(GACLentry *entry)
/* 
    GACLfreeEntry - free up space used by an entry (always returns 1;)
*/
{
  int i;
  
  if (entry == NULL) return 1;

  GACLfreeCreds(entry->firstcred);  

  free(entry);
  
  return 1;
}

int GACLfreeEntries(GACLentry *entry)
/*
    GACLfreeEntries - free up entry and all entries linked to in its *next 
                      chain
    (Private)
*/
{
  if (entry == NULL) return 0;
  
  if (entry->next != NULL) GACLfreeEntries(entry->next);
  
  return GACLfreeEntry(entry);  
}

int GACLinsertEntry(GACLentry *firstentry, GACLentry *newentry)
/* 
    (Private)
    
    GACLinsertEntry - insert an entry in the *next chain of firstentry

    FOR THE MOMENT THIS JUST APPENDS
*/
{
  if (firstentry == NULL) return 0;
  
  if (firstentry->next == NULL)
    {
      firstentry->next = newentry;
      return 1;
    }

  return GACLinsertEntry(firstentry->next, newentry);     
}

int GACLaddEntry(GACLacl *acl, GACLentry *entry)
/*  
    GACLaddEntry - add a new entry to an existing acl, returning 1
    on success or 0 on error 
*/ 
{
  if (acl == NULL) return 0;

  if (acl->firstentry == NULL) 
    { 
      acl->firstentry = entry;
      return 1;
    }
  else return GACLinsertEntry(acl->firstentry, entry);
}

int GACLprintEntry(GACLentry *entry, FILE *fp)
{
  GACLcred  *cred;
  GACLperm  i;
  
  fputs("<entry>\n", fp);
  
  for (cred = entry->firstcred; cred != NULL; cred = cred->next)
                                            GACLprintCred(cred, fp);

  if (entry->allowed)
    {
      fputs("<allow>", fp);

      for (i=GACL_PERM_READ; i <= GACL_PERM_ADMIN; ++i)
       if ((entry->allowed) & i) GACLprintPerm(i, fp);

      fputs("</allow>\n", fp);
    }
    

  if (entry->denied)
    {
      fputs("<deny>", fp);

      for (i=GACL_PERM_READ; i <= GACL_PERM_ADMIN; ++i)
       if (entry->denied & i) GACLprintPerm(i, fp);

      fputs("</deny>\n", fp);
    }
    
  fputs("</entry>\n", fp);

  return 1;
}

/*                                         *
 * Functions to manipulate GACLperm items *
 *                                         */

int GACLprintPerm(GACLperm perm, FILE *fp)
{
  GACLperm i;
  
  for (i=GACL_PERM_READ; gacl_perm_syms[i] != NULL; ++i)
       if (perm == gacl_perm_vals[i]) 
         {
           fprintf(fp, "<%s/>", gacl_perm_syms[i]);
           return 1;
         }
         
  return 0;
}

int GACLallowPerm(GACLentry *entry, GACLperm perm)
{
  entry->allowed = entry->allowed | perm;

  return 1;
}

int GACLunallowPerm(GACLentry *entry, GACLperm perm)
{
  entry->allowed = entry->allowed & ~perm;

  return 1;
}

int GACLdenyPerm(GACLentry *entry, GACLperm perm)
{
  entry->denied = entry->denied | perm;

  return 1;
}

int GACLundenyPerm(GACLentry *entry, GACLperm perm)
{
  entry->denied = entry->denied & ~perm;

  return 1;
}

char *GACLpermToChar(GACLperm perm)
/*
   GACLpermToChar - return char * or NULL corresponding to most significant
                     set bit of perm.
*/
{
  char      *p = NULL;
  GACLperm  i;
  
  for (i=0; gacl_perm_syms[i] != NULL; ++i)
       if (perm & gacl_perm_vals[i]) p = gacl_perm_syms[i];

  return p;
}

GACLperm GACLcharToPerm(char *s)
/*
   GACLcharToPerm - return access perm corresponding to symbol s[]
*/
{
  GACLperm i;

  for (i=0; gacl_perm_syms[i] != NULL; ++i)
       if (strcasecmp(gacl_perm_syms[i], s) == 0) return gacl_perm_vals[i];

  return -1; 
}

/*                                            *
 * Functions to manipulate GACLacl structures *
 *                                            */

GACLacl *GACLnewAcl(void)
/*  
    GACLnewAcl - allocate a new acl and return its pointer (or NULL 
                 on failure.)
*/
{
  GACLacl *newacl;
  
  newacl = (GACLacl *) malloc(sizeof(GACLacl));
  if (newacl == NULL) return NULL;
  
  newacl->firstentry = NULL;

  return newacl;
}

int GACLfreeAcl(GACLacl *acl)
/*
    GACLfreeAcl - free up space used by *acl. Always returns 1.
*/
{
  if (acl == NULL) return 1;

  GACLfreeEntries(acl->firstentry);  

  return 1;
}

int GACLprintAcl(GACLacl *acl, FILE *fp)
{
  GACLentry *entry;
  
  fputs("<gacl version=\"0.0.1\">\n", fp);
  
  for (entry = acl->firstentry; entry != NULL; entry = entry->next)
                                            GACLprintEntry(entry, fp);

  fputs("</gacl>\n", fp);

  return 1;
}

int GACLsaveAcl(char *filename, GACLacl *acl)
{
  int   ret;
  FILE *fp;
  
  fp = fopen(filename, "w");
  if (fp == NULL) return 0;
  
  fputs("<?xml version=\"1.0\"?>\n", fp);
  
  ret = GACLprintAcl(acl, fp);
  
  fclose(fp);
  
  return ret;
}

/*                                                    *
 * Functions for loading and parsing XML using libxml *
 *                                                    */
 
// need to check these for libxml memory leaks? - what needs to be freed?

GACLcred *GACLparseCred(xmlNodePtr cur)
/*
    GACLparseCred - parse a credential stored in the libxml structure cur, 
                    returning it as a pointer or NULL on error.
*/
{
  xmlNodePtr  cur2;
  GACLcred   *cred;
  
  cred = GACLnewCred((char *) cur->name);
  
  cred->firstname = NULL;
  cred->next      = NULL;
  
  for (cur2 = cur->xmlChildrenNode; cur2 != NULL; cur2=cur2->next)
     {
       if(xmlNodeIsText(cur2)) continue;
       GACLaddToCred(cred, (char *) cur2->name, 
                           (char *) xmlNodeGetContent(cur2));     
     }

  return cred;
}

GACLentry *GACLparseEntry(xmlNodePtr cur)
/*
    GACLparseEntry - parse an entry stored in the libxml structure cur,
                     returning it as a pointer or NULL on error.
*/
{
  int        i;
  xmlNodePtr cur2;
  GACLentry *entry;
  GACLcred  *cred;
  GACLperm  perm;

  if (xmlStrcmp(cur->name, (const xmlChar *) "entry") != 0) return NULL;
  
  cur = cur->xmlChildrenNode;
  
  entry = GACLnewEntry();
  
  while (cur != NULL)
       {
         if(xmlNodeIsText(cur)) { cur=cur->next; continue; };
         if (xmlStrcmp(cur->name, (const xmlChar *) "allow") == 0)
           {
             for (cur2 = cur->xmlChildrenNode; cur2 != NULL; cur2=cur2->next) {
               if(xmlNodeIsText(cur2)) continue;
               for (i=0; gacl_perm_syms[i] != NULL; ++i) {
                 if (xmlStrcmp(cur2->name, 
                             (const xmlChar *) gacl_perm_syms[i]) == 0)
                     GACLallowPerm(entry, gacl_perm_vals[i]);
	       }
	     }
           }
         else if (xmlStrcmp(cur->name, (const xmlChar *) "deny") == 0)
           {
             for (cur2 = cur->xmlChildrenNode; cur2 != NULL; cur2=cur2->next) {
               if(xmlNodeIsText(cur2)) continue;
               for (i=0; gacl_perm_syms[i] != NULL; ++i) {
                 if (xmlStrcmp(cur2->name, 
                             (const xmlChar *) gacl_perm_syms[i]) == 0)
                     GACLdenyPerm(entry, gacl_perm_vals[i]);
	       }
	     }
           }
         else if ((cred = GACLparseCred(cur)) != NULL) 
           {
             if (!GACLaddCred(entry, cred))
               { 
                 GACLfreeCred(cred);                
                 GACLfreeEntry(entry);
                 return NULL;
               }
           }
         else /* I cannot parse this - give up rather than get it wrong */
           {
             GACLfreeEntry(entry);
             return NULL;
           }
           
         cur=cur->next;
       } 
       
  return entry;
}

GACLacl *GACLloadAcl(char *filename)
{
  xmlDocPtr   doc;
  xmlNodePtr  cur;
  GACLacl    *acl;
  GACLentry  *entry;
        
  doc = xmlParseFile(filename);
  if (doc == NULL) return NULL;
    
  cur = xmlDocGetRootElement(doc);
  
  if (xmlStrcmp(cur->name, (const xmlChar *) "gacl"))
    {
      free(doc);
      free(cur);
      return NULL;
    }

  cur = cur->xmlChildrenNode;

  acl = GACLnewAcl();
  
  while (cur != NULL)
       {
         if(xmlNodeIsText(cur)) { cur=cur->next; continue; };
         entry = GACLparseEntry(cur);
         if (entry == NULL)
           {
             GACLfreeAcl(acl);
             xmlFreeDoc(doc);
             return NULL;
           }

         GACLaddEntry(acl, entry);
         
         cur=cur->next;
       }

  xmlFreeDoc(doc);
  return acl;
}

int GACLisAclFile(char *pathandfile)
/* Return 1 if filename in *pathandfile starts GACL_ACL_FILE 
   Return 0 otherwise. */
{ 
  char *filename;
      
  filename = rindex(pathandfile, '/');
  if (filename == NULL) filename = pathandfile;
  else                  filename++;
            
  return (strncmp(filename, GACL_ACL_FILE, sizeof(GACL_ACL_FILE) - 1) == 0);
}

GACLacl *GACLloadAclForFile(char *pathandfile)
/* Return ACL that governs the given file or directory (for directories,
   the GACL is in the directory itself.) */
{
  char        *path, *p;
  struct stat  statbuf;
  GACLacl     *acl;
  
  path = malloc(strlen(pathandfile) + sizeof(GACL_ACL_FILE) + 1);
  strcpy(path, pathandfile);

  if (stat(path, &statbuf) == 0)
    {
      if (!S_ISDIR(statbuf.st_mode)) /* can strip this / off straightaway */
        {
          p = rindex(path, '/');
          if (p != NULL) *p = '\0';
        }
    }

  while (path[0] != '\0')
       {
         strcat(path, "/");
         strcat(path, GACL_ACL_FILE);
         
         if (stat(path, &statbuf) == 0)
           {
             acl = GACLloadAcl(path);  
             free(path);  
             return acl;           
           }
           
         p = rindex(path, '/');
         *p = '\0';     /* strip off the / we added for ACL */

         p = rindex(path, '/');
         if (p == NULL) break; /* must start without / and we there now ??? */

         *p = '\0';     /* strip off another layer of / */                 
       }
       
  free(path);
  return NULL;
}

/*                                        *
 * Functions to create and query GACLuser *
 *                                        */

GACLuser *GACLnewUser(GACLcred *cred)
{
  GACLuser *user;
  
  if (cred == NULL) return NULL;
  
  user = malloc(sizeof(GACLuser));
  
  if (user != NULL) user->firstcred = cred;
  
  return user;
}

int GACLfreeUser(GACLuser *user)
{
  if (user == NULL) return 1;
  
  if (user->firstcred != NULL) GACLfreeCreds(user->firstcred);
  
  free(user);
  
  return 1;
}

int GACLuserAddCred(GACLuser *user, GACLcred *cred)
{
  GACLcred *crediter;

  if ((user == NULL) || (cred == NULL)) return 0;

  if (user->firstcred == NULL) 
    {
      user->firstcred = cred;
      cred->next = NULL; /* so cannot be used to add whole lists */
      return 1;
    }
  
  crediter = user->firstcred;  

  while (crediter->next != NULL) crediter = crediter->next;

  crediter->next = cred;
  cred->next = NULL; /* so cannot be used to add whole lists */
       
  return 1;
}

int GACLuserHasCred(GACLuser *user, GACLcred *cred)
/* test if the user has the given credential */
{
  GACLcred              *crediter;
  struct _GACLnamevalue *usernamevalue, *crednamevalue;

  if (cred == NULL) return 0;

  if (strcmp(cred->type, "any-user") == 0) return 1;
  
  if (user == NULL) return 0;
  
  if (strcmp(cred->type, "dn-list") == 0) 
    {
      if ((cred->firstname == NULL) ||
          (strcmp((cred->firstname)->name, "url") != 0) ||
          ((cred->firstname)->next != NULL))                 return 0;
      
      return GACLtestDnList((cred->firstname)->value, user);
    }
    
  if (strcmp(cred->type, "auth-user") == 0)
    {
      if ((user->firstcred == NULL) ||
          ((user->firstcred)->firstname == NULL)) return 0;
      
      for (crediter=user->firstcred; 
           crediter != NULL; 
           crediter = crediter->next)
        if (strcmp(crediter->type, "person") == 0) return 1;
                
      return 0;    
    }
 
  for (crediter=user->firstcred; crediter != NULL; crediter = crediter->next)
       {
         if (strcmp(crediter->type, cred->type) != 0) continue;
         
         if ((crediter->firstname == NULL) && 
             (cred->firstname     == NULL)) return 1;
         
         if ((crediter->firstname == NULL) || 
             (cred->firstname     == NULL)) continue;
             
         crednamevalue = cred->firstname;

         for (;crednamevalue;)
            {
              usernamevalue = crediter->firstname;
              for (;usernamevalue;)
                 {
                   if (strcmp(usernamevalue->name, crednamevalue->name ) == 0) { 
                        if (strcmp(usernamevalue->value?usernamevalue->value:"",
                                   crednamevalue->value?crednamevalue->value:"") == 0) {
                             break;
                        }
                   }
                   usernamevalue = usernamevalue->next;
                 }
              if(!usernamevalue) return 0;
              crednamevalue = crednamevalue->next;
            }
         return 1;
       }
  return 0;
}

GACLcred *GACLuserFindCredType(GACLuser *user, char *type)
/* find the first credential of a given type for this user */
{
  GACLcred *cred;

  if (user == NULL) return NULL;
  
  cred = user->firstcred;  

  while (cred != NULL)
       {
         if (strcmp(cred->type, type) == 0) return cred;
         
         cred = cred->next;       
       }
       
  return NULL;
}

/*                                                     *
 * Functions to test for access perm of an individual  *
 *                                                     */

int GACLtestDnList(char *listurl, GACLuser *user)
{
  char      *dn_lists_dir, *enclisturl, *filename, line[512], *p;
  FILE      *fp;
  GACLcred  *cred;
  
  if (listurl == NULL) return 0;

  dn_lists_dir = getenv("GACL_DN_LISTS");
  if (dn_lists_dir == NULL) dn_lists_dir = GACL_DN_LISTS;
  
  enclisturl = GACLurlEncode(listurl);
  
  filename = malloc(strlen(dn_lists_dir) + strlen(enclisturl) + 2);
  
  strcpy(filename, dn_lists_dir);
  strcat(filename, "/");
  strcat(filename, enclisturl);  
  free(enclisturl);
  
  fp = fopen(filename, "r");
  free(filename);
  
  if (fp == NULL) return 0;

  while (fgets(line, sizeof(line), fp) != NULL)
       {
         p = index(line, '\n');
         if (p != NULL) *p = '\0';

         cred = user->firstcred;
         
         while (cred != NULL)                  
              {
                if ((strcmp(cred->type, "person") == 0)          && 
                    (cred->firstname != NULL)                    &&
                    (strcmp("dn", (cred->firstname)->name) == 0) &&
                    (strcmp(line, (cred->firstname)->value?(cred->firstname)->value:"") == 0))
                  {
                    fclose(fp);
                    return 1;
                  }
                  
                cred = cred->next;
              }
       }
       
  fclose(fp);

  return 0;
}

GACLperm GACLtestUserAcl(GACLacl *acl, GACLuser *user)
/*
    GACLtestUserAcl - return bit fields depending on access perms user has
                      for given acl. All zero for no access. If *user is
                      NULL, matching to GACL_TYPE_ANYUSER will still work.
*/
{
  int        flag;
  GACLperm   allowperms = 0, denyperms = 0;
  GACLentry *entry;
  GACLcred  *cred, *usercred;
  
  if (acl == NULL) return 0;
  
  for (entry = acl->firstentry; entry != NULL; entry = entry->next)
     {
       flag = 1; /* assume this entry applies to us */
       
       /* now go through creds, checking they all do apply to us */
     
       for (cred = entry->firstcred; cred != NULL; cred = cred->next)
             if (!GACLuserHasCred(user, cred)) flag = 0;

       if (!flag) continue; /* flag false if a subtest failed */

       /* does apply to us, so we remember this entry's perms */
       
       allowperms = allowperms | entry->allowed;
       denyperms  = denyperms  | entry->denied;
     }

  return (allowperms & (~ denyperms)); 
  /* for each perm type, any deny we saw kills any allow */
}

GACLperm GACLtestExclAcl(GACLacl *acl, GACLuser *user)
/*
    GACLtestExclAcl - return bit fields depending on ALLOW perms OTHER users 
                      have for given acl. All zero if they have no access.
                      (used for testing if a user has exclusive access)
*/
{
  int        flag;
  GACLperm  perm = 0;
  GACLentry *entry;
  GACLcred  *cred;
  
  if (acl == NULL) return 0;
  
  for (entry = acl->firstentry; entry != NULL; entry = entry->next)
     {
       flag = 0; /* flag will be set if cred implies other users */
     
       for (cred = entry->firstcred; cred != NULL; cred = cred->next)
          {
            if (strcmp(cred->type, "person") != 0)
             /* if we ever add support for other person-specific credentials,
                they must also be recognised here */
              {
                flag = 1;
                break; 
              }

            if (!GACLuserHasCred(user, cred))
                 /* if user doesnt have this person credential, assume
                    it refers to a different individual */
              {
                flag = 1;
                break;
              }
          }

       if (flag) perm = perm | entry->allowed;
     }

  return perm;     
}

/*                                                            *
 * Utility functions, not necessarily specific to use of GACL *
 *                                                            */

char *GACLurlEncode(char *in)
/* Return a pointer to a malloc'd string holding a URL-encoded
   version of *in. Only A-Z a-z 0-9 . = - are passed through unmodified.
   (DN's processed by GACLurlEncode can be used as valid Unix filenames,
   assuming they do not exceed restrictions on filename length.) */
{
  char *out, *p, *q;
  
  out = malloc(3*strlen(in) + 1);
  
  p = in;
  q = out;
  
  while (*p != '\0')
       {
         if (isalnum(*p) || (*p == '.') || (*p == '=') || (*p == '-'))
           {
             *q = *p;
             ++q;
           }
         else
           {
             sprintf(q, "%%%2x", *p);
             q = &q[3];
           }

         ++p;
       }
  
  *q = '\0';  
  return out;
}

char *GACLmildUrlEncode(char *in)
/* Return a pointer to a malloc'd string holding a partially URL-encoded
   version of *in. "Partially" means that A-Z a-z 0-9 . = - _ @ and / 
   are passed through unmodified. (DN's processed by GACLmildUrlEncode()
   can be used as valid Unix paths+filenames if you are prepared to
   create or simulate the resulting /X=xyz directories.) */
{
  char *out, *p, *q;
  
  out = malloc(3*strlen(in) + 1);
  
  p = in;
  q = out;
  
  while (*p != '\0')
       {
         if (isalnum(*p) || (*p == '.') || (*p == '=') || (*p == '-') 
                         || (*p == '/') || (*p == '@') || (*p == '_'))
           {
             *q = *p;
             ++q;
           }
         else
           {
             sprintf(q, "%%%2x", *p);
             q = &q[3];
           }

         ++p;
       }
  
  *q = '\0';  
  return out;
}
