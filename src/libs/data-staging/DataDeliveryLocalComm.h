#ifndef DATADELIVERYLOCALCOMM_H_
#define DATADELIVERYLOCALCOMM_H_

#include <arc/Run.h>

#include "DataDeliveryComm.h"

namespace DataStaging {

  /// This class starts, monitors and controls a local Delivery process.
  /**
   * \ingroup datastaging
   * \headerfile DataDeliveryLocalComm.h arc/data-staging/DataDeliveryLocalComm.h
   */
  class DataDeliveryLocalComm : public DataDeliveryComm {
  public:

    /// Starts child process
    DataDeliveryLocalComm(DTR_ptr dtr, const TransferParameters& params);
    /// This stops the child process
    virtual ~DataDeliveryLocalComm();

    /// Returns "/" since local Delivery can access everywhere
    static bool CheckComm(DTR_ptr dtr, std::vector<std::string>& allowed_dirs, std::string& load_avg);

    /// Returns true if child process exists
    virtual operator bool() const { return (child_ != NULL); };
    /// Returns true if child process does not exist
    virtual bool operator!() const { return (child_ == NULL); };

  private:

    /// Polls every local transfer, from one thread.
    /**
     * Reading a child's pipe does not block and there is nothing to batch -
     * each transfer has its own child and its own pipe - so the poll is done
     * while holding the list lock. That keeps the lifetime question trivial:
     * Remove() cannot return while a poll is in progress, so a caller may
     * destroy the object as soon as it does.
     */
    class Poller {
     public:
      /// The one poller, created with its thread on first use
      static Poller& Instance();
      void Add(DataDeliveryLocalComm* item);
      /// Blocks until any poll in progress has finished
      void Remove(DataDeliveryLocalComm* item);
     private:
      Poller();
      Poller(const Poller&);
      Poller& operator=(const Poller&);
      static void func(void* arg);
      std::mutex lock_;
      std::list<DataDeliveryLocalComm*> items_;
    };

    /// Read from stdout of child to get status
    void PullStatus();

    /// Child process
    Arc::Run* child_;
    /// Stdin of child, used to pass credentials
    std::string stdin_;
    /// Temporary credentails location
    std::string tmp_proxy_;
    /// Time last communication was received from child
    Arc::Time last_comm;
  };

} // namespace DataStaging

#endif /* DATADELIVERYLOCALCOMM_H_ */
