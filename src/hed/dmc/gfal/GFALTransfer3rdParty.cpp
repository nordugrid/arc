#include "GFALUtils.h"

#include "GFALTransfer3rdParty.h"

namespace ArcDMCGFAL {

  using namespace Arc;

  Logger GFALTransfer3rdParty::logger(Logger::getRootLogger(), "Transfer3rdParty");

  // Callback passed to gfal. It calls DataPoint callback and fills relevant
  // info. DataPoint pointer is stored in user_data.
  void GFALTransfer3rdParty::gfal_3rd_party_callback(gfalt_transfer_status_t h, const char* src, const char* dst, gpointer user_data) {
    DataPoint::TransferCallback* cb = (DataPoint::TransferCallback*)user_data;
    if (cb && *cb) {
      GError * err = NULL;
      size_t bytes = gfalt_copy_get_bytes_transfered(h, &err);
      if (err != NULL) {
        logger.msg(WARNING, "Failed to obtain bytes transferred: %s", err->message);
        g_error_free(err);
        return;
      }
      (*(*cb))(bytes);
    }
  }

  GFALTransfer3rdParty::GFALTransfer3rdParty(const URL& src, const URL& dest,
                                             const Arc::UserConfig& cfg, DataPoint::TransferCallback cb)
  : source(src), destination(dest), transfer_timeout(cfg.Timeout()), callback(cb) {};

  DataStatus GFALTransfer3rdParty::Transfer() {

    if (!source) return DataStatus(DataStatus::TransferError, EINVAL, "Invalid source URL");
    if (!destination) return DataStatus(DataStatus::TransferError, EINVAL, "Invalid destination URL");

    GError * err = NULL;
    int error_no = EARCOTHER;

    // Set up parameters and options
    gfalt_params_t params = gfalt_params_handle_new(&err);
    if (err != NULL) {
      logger.msg(VERBOSE, "Failed to get initiate GFAL2 parameter handle: %s", err->message);
      g_error_free(err);
      return DataStatus(DataStatus::TransferError, error_no, "Failed to get initiate GFAL2 parameter handle");
    }

    gfal2_context_t ctx = gfal2_context_new(&err);
    if (err != NULL) {
      logger.msg(VERBOSE, "Failed to get initiate new GFAL2 context: %s", err->message);
      g_error_free(err);
      return DataStatus(DataStatus::TransferError, error_no, "Failed to get initiate new GFAL2 context");
    }

    gfalt_add_monitor_callback(params, &gfal_3rd_party_callback, (gpointer)(&callback), NULL, &err);
    if (err != NULL) {
      logger.msg(VERBOSE, "Failed to set GFAL2 monitor callback: %s", err->message);
      g_error_free(err);
      return DataStatus(DataStatus::TransferError, error_no, "Failed to set GFAL2 monitor callback");
    }

    // Set replace according to overwrite option
    gfalt_set_replace_existing_file(params, (destination.Option("overwrite") == "yes"), &err);
    if (err != NULL) {
      logger.msg(VERBOSE, "Failed to set overwrite option in GFAL2: %s", err->message);
      g_error_free(err);
      return DataStatus(DataStatus::TransferError, error_no, "Failed to set overwrite option in GFAL2");
    }

    // Set transfer timeout
    gfalt_set_timeout(params, transfer_timeout, &err);
    if (err != NULL) {
      logger.msg(WARNING, "Failed to set GFAL2 transfer timeout, will use default: %s", err->message);
      g_error_free(err);
      err = NULL;
    }

    // Do the copy
    int res = gfalt_copy_file(ctx, params, GFALUtils::GFALURL(source).c_str(),
                              GFALUtils::GFALURL(destination).c_str(), &err);
    gfal2_context_free(ctx);
    gfalt_params_handle_delete(params, NULL);

    if (res != 0) {
      logger.msg(VERBOSE, "Transfer failed");
      if (err != NULL) {
        logger.msg(VERBOSE, err->message);
        error_no = err->code;
        g_error_free(err);
      }
      return DataStatus(DataStatus::TransferError, error_no);
    }
    logger.msg(INFO, "Transfer succeeded");
    return DataStatus::Success;
  }

} // namespace ArcDMCGFAL
