#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

namespace ArcCredential {


char **listadd(char **vect, char *data, int size) {
  int i = 0;
  char **newvect;

  if (!data || (size <= 0))
    return NULL;

  if (vect)
    while (vect[i++]) ;
  else
    i=1;

  if ((newvect = (char **)malloc((i+1)*size))) {
    if (vect) {
      memcpy(newvect, vect, (size*(i-1)));
      newvect[i-1] = data;
      newvect[i] = NULL;
      free(vect);
    }
    else {
      newvect[0] = data;
      newvect[1] = NULL;
    }
    return newvect;
  }
  return NULL;
}

void listfree(char **vect, void (*f)(void *)) {
  char **tmp = vect;
  if (tmp) {
    int i = 0;
    while (tmp[i])
      f(tmp[i++]);
    free(vect);
  }
}


} // namespace ArcCredential

