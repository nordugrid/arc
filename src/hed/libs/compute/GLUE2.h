// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_GLUE2_H__
#define __ARC_GLUE2_H__

#include <list>
#include <string>

#include <arc/compute/ExecutionTarget.h>

namespace Arc {

  /// GLUE2 parser
  /**
   * This class parses GLUE2 information rendeed in XML and transfers
   * information into various classes representing different types
   * of objects which GLUE2 information model can describe.
   * This parser uses GLUE Specification v. 2.0 (GFD-R-P.147).
   */
  class GLUE2 {
  public:
    /**
     * Parses ComputingService elements of GLUE2 into ComputingServiceType objects.
     * The glue2tree is either XML tree representing ComputingService object
     * directly or ComputingService objects are immediate children of it.
     * On exit targets contains ComputingServiceType objects found inside glue2tree.
     * If targets contained any objects on entry those are not destroyed.
     *
     * @param glue2tree
     * @param targets
     */
    static void ParseExecutionTargets(XMLNode glue2tree, std::list<ComputingServiceType>& targets);
  private:
    static Logger logger;
  };

}

#endif // __ARC_GLUE2_H__
