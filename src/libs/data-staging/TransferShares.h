#ifndef TRANSFERSHARES_H_
#define TRANSFERSHARES_H_

#include <map>

#include <arc/Thread.h>
#include <arc/credential/VOMSUtil.h>

#include "DTR.h"

namespace DataStaging {

  /// TransferSharesConf describes the configuration of TransferShares.
  /**
   * It allows reference shares to be defined with certain priorities. An
   * instance of this class is used when creating a TransferShares object.
   */
  class TransferSharesConf {

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

    /// ReferenceShares are special shares defined in the configuration with
    /// specific priorities. The "_default" share must always be defined.
    std::map<std::string, int> ReferenceShares;

    /// Configured share type
    ShareType shareType;

    /// Find the name of the share this dtr belongs to, when a USER ShareType is used
    std::string extract_user_share(const Arc::Credential& cred){
      return getCredentialProperty(cred, "dn");
    }

    /// Find the name of the share this dtr belongs to, when a VO ShareType is used
    std::string extract_vo_share(const Arc::Credential& cred){
      return getCredentialProperty(cred, "voms:vo");
    }

    /// Find the name of the share this dtr belongs to, when a GROUP ShareType is used
    std::string extract_group_share(const Arc::Credential& cred){
      return getCredentialProperty(cred, "voms:group");
    }

    /// Find the name of the share this dtr belongs to, when a ROLE ShareType is used
    std::string extract_role_share(const Arc::Credential& cred){
      return getCredentialProperty(cred, "voms:role");
    }

  public:

    /// Construct a new TransferSharesConf with given share type and reference shares
    TransferSharesConf(const std::string& type, const std::map<std::string, int>& ref_shares);

    /// Construct a new TransferSharesConf with no defined shares or policy
    TransferSharesConf();

    /// Set the share type
    void set_share_type(const std::string& type);

    /// Add a reference share
    void set_reference_share(const std::string& RefShare, int Priority);

    /// Set reference shares
    void set_reference_shares(const std::map<std::string, int>& shares);

    /// Returns true if the given share is a reference share
    bool is_configured(const std::string& ShareToCheck);

    /// Get the priority of this share
    int get_basic_priority(const std::string& ShareToCheck);

    /// Return human-readable configuration of shares
    std::string conf() const;

    /// Get the name of the share the DTR should be assigned to
    std::string extract_share_info(DTR_ptr DTRToExtract);
  };


  /// TransferShares is used to implement fair-sharing and priorities.
  /**
   * TransferShares defines the algorithm used to prioritise and share
   * transfers among different users or groups. Configuration information on
   * the share type and reference shares is held in a TransferSharesConf
   * instance. The Scheduler uses TransferShares to determine which DTRs in the
   * queue for each process go first. The calculation is based on the
   * configuration and the currently active shares (the DTRs already in the
   * process). can_start() is the method called by the Scheduler to
   * determine whether a particular share has an available slot in the process.
   */
  class TransferShares {

   private:

    /// Configuration of share type and reference shares
    TransferSharesConf conf;

    /// Shares which are active, ie running or in the queue, and number of DTRs
    std::map<std::string, int> ActiveShares;

    /// How many transfer slots each active share can grab
    std::map<std::string, int> ActiveSharesSlots;

   public:

    /// Create a new TransferShares with default configuration
    TransferShares() {};

    /// Create a new TransferShares with given configuration
    TransferShares(const TransferSharesConf& shares_conf);

    /// Empty destructor
    ~TransferShares(){};

    /// Copy constructor must be defined because SimpleCondition cannot be copied
    TransferShares(const TransferShares& shares);
    /// Assignment operator must be defined because SimpleCondition cannot be copied
    TransferShares& operator=(const TransferShares& shares);

    /// Set a new configuration, if a new reference share gets added for example
    void set_shares_conf(const TransferSharesConf& share_conf);

    /// Calculate how many slots to assign to each active share.
    /** This method is called each time the Scheduler loops to calculate the
     * number of slots to assign to each share, based on the current number
     * of active shares and the shares' relative priorities. */
    void calculate_shares(int TotalNumberOfSlots);

    /// Increase by one the active count for the given share.
    /** Called when a new DTR enters the queue. */
    void increase_transfer_share(const std::string& ShareToIncrease);
    /// Decrease by one the active count for the given share
    /** Called when a completed DTR leaves the queue. */
    void decrease_transfer_share(const std::string& ShareToDecrease);

    /// Decrease by one the number of slots available to the given share.
    /** Called when there is a slot already used by this share
     * to reduce the number available. */
    void decrease_number_of_slots(const std::string& ShareToDecrease);

    /// Returns true if there is a slot available for the given share
    bool can_start(const std::string& ShareToStart);

    /// Returns the map of active shares
    std::map<std::string, int> active_shares() const;

  }; // class TransferShares

} // namespace DataStaging

#endif /* TRANSFERSHARES_H_ */
