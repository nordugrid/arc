// -*- indent-tabs-mode: nil -*-

#ifndef ARCLIB_JSON
#define ARCLIB_JSON

#include <arc/XMLNode.h>

namespace Arc {

  /// Holder class for parsing JSON into XML container and back.
  class JSON {
   public:
    /// Constructor is not implemented. Use static methods instead.
    JSON();
    ~JSON();


    /// Parse JSON document and store results into XMLNode container.
    static bool Parse(Arc::XMLNode& xml, char const * input);

   private:
    static char const * ParseInternal(Arc::XMLNode& xml, char const * input, int depth);
  };

} // namespace Arc

#endif // ARCLIB_JSON

