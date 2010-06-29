/*

   Example program using GACL http://www.gridpp.ac.uk/gacl/

   Build with  gcc -o gaclexample gaclexample.c -lgacl -lxml2 -lz -lm

*/

#include <stdio.h>
#include <gacl.h>

main()
{
  GACLcred  *cred, *usercred;
  GACLentry *entry;
  GACLacl   *acl1, *acl2;
  GACLuser  *user;

  /* must initialise GACL before using it */
  
  GACLinit();

  /* build up an ACL, starting with a credential */

  cred = GACLnewCred("person");

  GACLaddToCred(cred, "dn", "/O=Grid/CN=Mr Grid Person");
  
  /* create an entry to put it in */ 
   
  entry = GACLnewEntry();
  
  /* add the credential to it */
  
  GACLaddCred(entry, cred);
  
  /* associate some permissions and denials to the credential */  
  
  GACLallowPerm( entry, GACL_PERM_READ);
  GACLallowPerm( entry, GACL_PERM_WRITE);
  GACLallowPerm( entry, GACL_PERM_ADMIN);
  GACLdenyPerm(  entry, GACL_PERM_ADMIN);
  GACLdenyPerm(  entry, GACL_PERM_LIST);

  /* create a new ACL and add the entry to it */
  
  acl1 = GACLnewAcl();
  
  GACLaddEntry(acl1, entry);

  /* create a GACLuser to compare with the ACL */

  usercred = GACLnewCred("person");
  
  GACLaddToCred(usercred, "dn", "/O=Grid/CN=Mr Grid Person");
   
  user = GACLnewUser(usercred);

  printf("test /O=Grid/CN=Mr Grid Person in acl = %d\n",
          GACLtestUserAcl(acl1, user));

  /* print and save the whole ACL */

  GACLprintAcl(acl1, stdout);
  
  GACLsaveAcl("example.gacl", acl1);

  puts("gridacl.out saved");

  puts("");

  /* load the ACL back off the disk, print and test it */

  acl2 = GACLloadAcl("example.gacl");

  puts("gridacl.out loaded");
  
  if (acl2 != NULL) GACLprintAcl(acl2, stdout); else puts("acl2 is NULL");

  printf("test /O=Grid/CN=Mr Grid Person in acl = %d\n",
          GACLtestUserAcl(acl2, user));
}
