#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "arex.h"

namespace ARex {

Arc::MCC_Status ARexService::CacheCheck(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
   // TODO: Finish this method
   //
   // Method for grid-manager cache checking 
   //
   //  You should call CheckCreated() with the URL of the file you want to check - eg CheckCreated("http://..."). File() is called within this method to resolve the URL to the local file name.
   //
   //  This information is from David Cameron
   //
   
   return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex 
