#ifndef ARCLIB_ALIAS
#define ARCLIB_ALIAS

#include <list>
#include <string>

/**
 * Replaces cluster aliases in a cluster list with the resolved cluster names.
 * Note that since an alias can resolve to more than one cluster name the list
 * can be longer after a call to this function.
 *
 * Alias definitions are read in the following order:
 *   1. the [common] block of /etc/arc.conf
 *   2. the [client] block of /etc/arc.conf
 *   3. the [common] block of ${NORDUGRID_LOCATION}/etc/arc.conf
 *   4. the [client] block of ${NORDUGRID_LOCATION}/etc/arc.conf
 *   5. the [common] block of ${HOME}/.arc/client.conf
 *   6. the [client] block of ${HOME}/.arc/client.conf
 *   7. the ${HOME}/.ngalias file
 *
 * Alias definitions are recursive. Any alias defined in a block that is read
 * before a given block can be used in alias definitions in that block. An
 * alias defined in a block can also be used in alias definitions later in the
 * same block.
 */
void ResolveAliases(std::list<std::string>& clusters);

#endif // ARCLIB_ALIAS
