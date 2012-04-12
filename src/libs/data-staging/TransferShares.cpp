#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TransferShares.h"

#include <math.h>

#include <arc/StringConv.h>

namespace DataStaging {

  TransferSharesConf::TransferSharesConf(const std::string& type, const std::map<std::string, int>& ref_shares) {
    set_share_type(type);
    set_reference_shares(ref_shares);
  }

  TransferSharesConf::TransferSharesConf() : shareType(NONE) {
    ReferenceShares["_default"] = 50;
  }

  void TransferSharesConf::set_share_type(const std::string& type) {
    if (Arc::lower(type) == "dn")
      shareType = USER;
    else if (Arc::lower(type) == "voms:vo")
      shareType = VO;
    else if (Arc::lower(type) == "voms:role")
      shareType = ROLE;
    else if (Arc::lower(type) == "voms:group")
      shareType = GROUP;
    else
      shareType = NONE;
  }

  bool TransferSharesConf::is_configured(const std::string& ShareToCheck) {
    return (ReferenceShares.find(ShareToCheck) != ReferenceShares.end());
  }

  int TransferSharesConf::get_basic_priority(const std::string& ShareToCheck) {
    if (!is_configured(ShareToCheck))
      return ReferenceShares["_default"];
    return ReferenceShares[ShareToCheck];
  }

  void TransferSharesConf::set_reference_share(const std::string& RefShare, int Priority) {
    ReferenceShares[RefShare] = Priority;
  }

  void TransferSharesConf::set_reference_shares(const std::map<std::string, int>& shares) {
    ReferenceShares = shares;
    // there should always be a _default share defined
    if (ReferenceShares.find("_default") == ReferenceShares.end())
      ReferenceShares["_default"] = 50;
  }

  std::string TransferSharesConf::conf() const {
    std::string conf;
    conf += " Share type: ";
    switch (shareType){
      case USER: conf += "DN"; break;
      case VO: conf += "VOMS VO"; break;
      case GROUP: conf += "VOMS group"; break;
      case ROLE: conf += "VOMS role"; break;
      case NONE: conf += "None"; break;
      default: // Something really strange
        conf += "unknown"; break;
    }
    if (!ReferenceShares.empty()) {
      for (std::map<std::string, int>::const_iterator i = ReferenceShares.begin(); i != ReferenceShares.end(); ++i) {
        conf += "\n Reference share " + i->first + ", priority " + Arc::tostring(i->second);
      }
    }
    return conf;
  }

  std::string TransferSharesConf::extract_share_info(DTR_ptr DTRToExtract) {
    Arc::Credential cred(DTRToExtract->get_usercfg().ProxyPath(),
                         DTRToExtract->get_usercfg().ProxyPath(),
                         DTRToExtract->get_usercfg().CACertificatesDirectory(), "");

    using namespace ArcCredential; // needed for marco expansion
    if (CERT_IS_RFC_PROXY(cred.GetType())) DTRToExtract->set_rfc_proxy(true);

    switch (shareType){
      case USER: return extract_user_share(cred);
      case VO: return extract_vo_share(cred);
      case GROUP: return extract_group_share(cred);
      case ROLE: return extract_role_share(cred);
      case NONE: return "_default";
      default: // Something really strange
        return "";
    }
  }


  TransferShares::TransferShares(const TransferSharesConf& shares_conf) :
    conf(shares_conf) {
    ActiveShares.clear();
    ActiveSharesSlots.clear();
  }

  TransferShares::TransferShares(const TransferShares& shares) :
    conf(shares.conf),
    ActiveShares(shares.ActiveShares),
    ActiveSharesSlots(shares.ActiveSharesSlots)
  {}

  TransferShares& TransferShares::operator=(const TransferShares& shares) {
    conf = shares.conf;
    ActiveShares = shares.ActiveShares;
    ActiveSharesSlots = shares.ActiveSharesSlots;
    return *this;
  }

  void TransferShares::set_shares_conf(const TransferSharesConf& shares_conf) {
    conf = shares_conf;
  }

  void TransferShares::calculate_shares(int TotalNumberOfSlots) {

    ActiveSharesSlots.clear();
    // clear active shares with 0 count
    // and compute the summarized priority of other active shares
    std::map<std::string, int>::iterator i;
    int SummarizedPriority = 0;
    int TotalQueued = 0;
    for (i = ActiveShares.begin(); i != ActiveShares.end(); ){
      if (i->second == 0) {
        ActiveShares.erase(i++);
      } else {
        SummarizedPriority += conf.get_basic_priority(i->first);
        TotalQueued += i->second;
        ++i;
      }
    }

    int slots_used = 0;
    // first calculate shares based on the share priority
    for (i = ActiveShares.begin(); i != ActiveShares.end(); i++){
      // Number of slots for this share is its priority divided by total
      // priorities of all active shares multiplied by the total number of slots
      int slots = int(::floor(float(conf.get_basic_priority(i->first)) / float(SummarizedPriority) * float(TotalNumberOfSlots)));

      if (slots > i->second) {
        // Don't assign more slots than the share needs
        ActiveSharesSlots[i->first] = i->second;
      }
      else if (slots == 0) {
        // Some shares can receive 0 slots.
        // It can happen when there are lots of shares active
        // or one share has enormously big priority.
        // There should be no 0 in the number of slots, so every
        // share has at least theoretical possibility to start
        ActiveSharesSlots[i->first] = 1;
      }
      else {
        ActiveSharesSlots[i->first] = slots;
      }
      slots_used += ActiveSharesSlots[i->first];
    }
    // now assign unused slots among shares with more DTRs than slots
    while (slots_used < TotalQueued && slots_used < TotalNumberOfSlots) {
      // TODO share slots using priorities
      for (i = ActiveShares.begin(); i != ActiveShares.end(); i++){
        if (ActiveSharesSlots[i->first] < ActiveShares[i->first]) {
          ActiveSharesSlots[i->first]++;
          slots_used++;
          if (slots_used >= TotalQueued || slots_used >= TotalNumberOfSlots) break;
        }
      }
    }
  }

  void TransferShares::increase_transfer_share(const std::string& ShareToIncrease) {
    ActiveShares[ShareToIncrease]++;
  }

  void TransferShares::decrease_transfer_share(const std::string& ShareToDecrease) {
    ActiveShares[ShareToDecrease]--;
  }

  void TransferShares::decrease_number_of_slots(const std::string& ShareToDecrease) {
    ActiveSharesSlots[ShareToDecrease]--;
  }

  bool TransferShares::can_start(const std::string& ShareToStart) {
    return (ActiveSharesSlots[ShareToStart] > 0);
  }

  std::map<std::string, int> TransferShares::active_shares() const {
    return ActiveShares;
  }

}
