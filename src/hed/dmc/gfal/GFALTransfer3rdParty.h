#ifndef TRANSFER3RDPARTY_H_
#define TRANSFER3RDPARTY_H_

#include <gfal_api.h>

#include <arc/data/DataPoint.h>

namespace ArcDMCGFAL {

  using namespace Arc;

  /// Class to interact with GFAL2 to do third-party transfer
  class GFALTransfer3rdParty {
  public:
    /// Constructor
    GFALTransfer3rdParty(const URL& source, const URL& dest,
                         const Arc::UserConfig& cfg, DataPoint::TransferCallback cb);
    /// Start transfer
    DataStatus Transfer();
  private:
    URL source;
    URL destination;
    int transfer_timeout;
    DataPoint::TransferCallback callback;
    static Logger logger;
    /// Callback that is passed to GFAL2. It calls our Callback3rdParty callback
    static void gfal_3rd_party_callback(gfalt_transfer_status_t h, const char* src, const char* dst, gpointer user_data);
  };

} // namespace ArcDMCGFAL


#endif /* TRANSFER3RDPARTY_H_ */
