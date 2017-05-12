/**Borrow the code about list operation from VOMS, specially for list of Attribute Certificate. 
 * Because the AC** operator is required for i2d_AC() and d2i_AC() method, and implicitly required by OpenSSL code,
 * some other solution like std::list<AC*> is not suitable here.
 */

/**The listfunc.h and listfunc.h are introduced from code written by VOMS project,
 *so here the original license follows.
 */

#ifndef ARC_LISTFUNC_H
#define ARC_LISTFUNC_H

namespace ArcCredential {

#include <stdlib.h>

typedef void (*freefn)(void *);

/* Merges element data to NULL-terminated array vect. 
  Returns pointer to new merged array. Old vect array is destroyed.
  size is the size of array element and for safety should always
  be sizeof(char*) */
extern char **listadd(char **vect, char *data, int size);

/* Frees memory associated with array vect all data which its 
  elements are pointing to. For freeing pointed data supplied 
  function f is used. On exit vect array is destroyed. */
extern void   listfree(char **vect, freefn f);


} // namespace ArcCredential

#endif
