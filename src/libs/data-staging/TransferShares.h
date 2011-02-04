#ifndef TRANSFERSHARES_H_
#define TRANSFERSHARES_H_

#include <map>

#include <arc/Thread.h>
#include <arc/credential/VOMSUtil.h>

#include "DTR.h"

namespace DataStaging {

/**
 * TransferShares defines the algorithm used to prioritise and share transfers
 * among different users or groups. It contains configuration configuration
 * information on the share type and reference shares. The Scheduler uses
 * TransferShares to determine which DTRs in the queue for Delivery go first
 * to Delivery. The calculation is based on the configuration and the currently
 * active shares (the DTRs already in Delivery). can_start() is the method
 * called by the Scheduler to determine whether a particular share has an
 * available slot in Delivery.
 */
class TransferShares {

  public:

    /// The criterion for assigning a share to a DTR
    enum ShareType {
      /// Shares are defined per DN of the user's proxy
      USER,
      /// Shares are defined per VOMS VO of the user's proxy
      VO,
      /// Shares are defined per VOMS group of the user's proxy
      GROUP,
      /// Shares are defined per VOMS role of the user's proxy
      ROLE,
      /// No share criterion - all DTRs will be assigned to a single share
      NONE
    };

  private:

    /* Lock to prevent race conditions in transfer shares
     */
    Arc::SimpleCondition SharesLock;

    /* Which type of share are we using now.
     * The configuration module need to change it
     */
    ShareType shareType;

    /* ReferenceShares are populated by the configuration module.
     * They contain the name of the share and its priority.
     * The configuration module should take care of inserting
     * "_default" share with default priority here
     */
    std::map<std::string, int> ReferenceShares;

    /* ActiveShares contain the shares that are active at the moment,
     * i.e. shares from ReferenceShares that have active DTRs
     * belonging to them
     */
    std::map<std::string, int> ActiveShares;

    /* How many transfer slots each of active shares can grab
     */
    std::map<std::string, int> ActiveSharesSlots;

    /* Find the name of the share this dtr belongs to
     */
    std::string extract_user_share(const Arc::Credential& cred){
      return get_property(cred, "dn");
    }

    std::string extract_vo_share(const Arc::Credential& cred){
      return get_property(cred, "voms:vo");
    }

    std::string extract_group_share(const Arc::Credential& cred){
      return get_property(cred, "voms:group");
    }

    std::string extract_role_share(const Arc::Credential& cred){
      return get_property(cred, "voms:role");
    }

  public:

    /// Create a new TransferShares with no configuration
    TransferShares();
    ~TransferShares(){};

    /// Copy constructor must be defined because SimpleCondition cannot be copied
    TransferShares(const TransferShares& shares);
    /// Assignment operator must be defined because SimpleCondition cannot be copied
    TransferShares operator=(const TransferShares& shares);

    /// Get the name of the share the DTR should be assigned to
    std::string extract_share_info(const DTR& DTRToExtract);

    /// Calculate how many slots to assign to each active share
    /** This method is called each time the Scheduler loops to calculate the
     * number of slots to assign to each share, based on the current number
     * of active shares and the shares' relative priorities. */
    void calculate_shares(int TotalNumberOfSlots);

    /// Increase by one the active count for the given share
    /** Called when a new DTR enters the system. */
    void increase_transfer_share(const std::string& ShareToIncrease);
    /// Decrease by one the active count for the given share
    /** Called when a completed DTR leaves the system. */
    void decrease_transfer_share(const std::string& ShareToDecrease);

    /// Decrease by one the number of slots available to the given share
    /** Called when there is a Delivery slot already used by this share
     * to reduce the number available. */
    void decrease_number_of_slots(const std::string& ShareToDecrease);

    /// Returns true if there is a slot available for the given share
    bool can_start(const std::string& ShareToStart);

    /// Returns true if the given share is a reference share
    bool is_configured(const std::string& ShareToCheck);

    /// Get the priority of this share
    int get_basic_priority(const std::string& ShareToCheck);

    /// Add a reference share
    void set_reference_share(const std::string& RefShare, int Priority);

    /// Set reference shares
    void set_reference_shares(const std::map<std::string, int>& shares);

    /// Set the share type
    void set_share_type(ShareType Type);

    /// Set the share type
    void set_share_type(const std::string& type);

    /// Return human-readable configuration of shares
    std::string conf() const;

}; // class TransferShares

} // namespace DataStaging

#endif /* TRANSFERSHARES_H_ */
