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

  std::string TransferSharesConf::extract_share_info(const DTR& DTRToExtract) {
    if (shareType == NONE) return "_default";
    Arc::Credential cred(DTRToExtract.get_usercfg().ProxyPath(),
                         DTRToExtract.get_usercfg().ProxyPath(),
                         DTRToExtract.get_usercfg().CACertificatesDirectory(), "");
    switch (shareType){
      case USER: return extract_user_share(cred);
      case VO: return extract_vo_share(cred);
      case GROUP: return extract_group_share(cred);
      case ROLE: return extract_role_share(cred);
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
    SharesLock(),
    conf(shares.conf),
    ActiveShares(shares.ActiveShares),
    ActiveSharesSlots(shares.ActiveSharesSlots)
  {}

  TransferShares& TransferShares::operator=(const TransferShares& shares) {
    //SharesLock = Arc::SimpleCondition();
    conf = shares.conf;
    ActiveShares = shares.ActiveShares;
    ActiveSharesSlots = shares.ActiveSharesSlots;
    return *this;
  }

  void TransferShares::set_shares_conf(const TransferSharesConf& shares_conf) {
    SharesLock.lock();
    conf = shares_conf;
    SharesLock.unlock();
  }

  void TransferShares::calculate_shares(int TotalNumberOfSlots){

    SharesLock.lock();
    ActiveSharesSlots.clear();
    // clear active shares with 0 count
    // and compute the summarized priority of other active shares
    std::map<std::string, int>::iterator i;
    int SummarizedPriority = 0;
    for (i = ActiveShares.begin(); i != ActiveShares.end(); ){
      if (i->second == 0) {
        ActiveShares.erase(i++);
      } else {
        SummarizedPriority += conf.get_basic_priority(i->first);
        ++i;
      }
    }

    int temp;
    for (i = ActiveShares.begin(); i != ActiveShares.end(); i++){
      // Number of slots for this share is its priority divided by total
      // priorities of all active shares multiplied by the total number of slots
      temp = int(::floor(float(conf.get_basic_priority(i->first)) / float(SummarizedPriority) * float(TotalNumberOfSlots)));

      // Some shares can receive 0 slots.
      // It can happen when there are lots of shares active
      // or one share has enormously big priority.
      // There should be no 0 in the number of slots, so every
      // share has at least theoretical possibility to start
      ActiveSharesSlots[i->first] = (temp == 0 ? 1 : temp);
    }
    SharesLock.unlock();
  }

  void TransferShares::increase_transfer_share(const std::string& ShareToIncrease){
    SharesLock.lock();
    ActiveShares[ShareToIncrease]++;
    SharesLock.unlock();
  }

  void TransferShares::decrease_transfer_share(const std::string& ShareToDecrease){
    SharesLock.lock();
    ActiveShares[ShareToDecrease]--;
    SharesLock.unlock();
  }

  void TransferShares::decrease_number_of_slots(const std::string& ShareToDecrease){
    SharesLock.lock();
    ActiveSharesSlots[ShareToDecrease]--;
    SharesLock.unlock();
  }

  bool TransferShares::can_start(const std::string& ShareToStart){
    SharesLock.lock();
    int slots_available = ActiveSharesSlots[ShareToStart];
    SharesLock.unlock();
    return (slots_available > 0);
  }

}
