/**Borrow the code about list operation from VOMS, specially for list of Attribute Certificate. 
 * Because the AC** operator is required for i2d_AC() and d2i_AC() method, and implicitly required by OpenSSL code,
 * some other solution like std::list<AC*> is not suitable here.
 */

/**The listfunc.h and listfunc.h are introduced from code written by VOMS project,
 *so here the original license follows.
 */

#ifndef ARC_LISTFUNC_H
#define ARC_LISTFUNC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef void (*freefn)(void *);

extern char **listjoin(char **base, char **addon, int size);
extern char **listadd(char **vect, char *data, int size);
extern void   listfree(char **vect, freefn f);

#ifdef __cplusplus
}
#endif

#endif
