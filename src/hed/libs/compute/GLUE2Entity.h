// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_GLUE2ENTITY_H__
#define __ARC_GLUE2ENTITY_H__

/** \file
 * \brief template class for %GLUE2 entities.
 */

#include <arc/Utils.h>

namespace Arc {

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  template<typename T>
  class GLUE2Entity {
  public:
    GLUE2Entity() : Attributes(new T) {}

    T       * operator->()       { return &(*Attributes); }
    T const * operator->() const { return &(*Attributes); }
    T       & operator*()        { return *Attributes; }
    T const & operator*()  const { return *Attributes; }

    CountedPointer<T> Attributes;
  };
} // namespace Arc

#endif // __ARC_GLUE2ENTITY_H__
