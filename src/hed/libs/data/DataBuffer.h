// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATABUFFER_H__
#define __ARC_DATABUFFER_H__

#include <glibmm/thread.h>

#include <arc/data/DataSpeed.h>

namespace Arc {

  class CheckSum;

  /// Represents set of buffers
  /** This class is used used during data transfer using DataPoint classes. */
  class DataBuffer {
  private:
    /* used to check if configuration changed */
    int set_counter;
    /* general purpose mutex and condition used to
       achieve thread safety */
    Glib::Mutex lock;
    Glib::Cond cond;
    /* structure do describe status of every buffer */
    typedef struct {
      /* buffer address in memory */
      char *start;
      /* true if taken by application for filling */
      bool taken_for_read;
      /* true if taken by application for emptying */
      bool taken_for_write;
      /* size of buffer */
      unsigned int size;
      /* amount of information stored */
      unsigned int used;
      /* offset in file or similar, has meaning only for application */
      unsigned long long int offset;
    } buf_desc;
    /* amount of data passed through buffer (including current stored).
       computed using offset and size. gaps are ignored. */
    unsigned long long int eof_pos;
    /* list of controlled buffers */
    buf_desc *bufs;
    /* amount of controlled buffers */
    int bufs_n;
    /* set to true if application's reading(filling) part
       won't use buffer anymore */
    bool eof_read_flag;
    /* same for writing(emptying) part */
    bool eof_write_flag;
    /* reading part of application experienced error */
    bool error_read_flag;
    /* same for writing part */
    bool error_write_flag;
    /* error was originated in DataBuffer itself */
    bool error_transfer_flag;
    /* wait for any change of buffers' status */
    bool cond_wait();
    /* pointer to object to compute checksum */
    typedef struct checksum_desc
    {
      checksum_desc(CheckSum * sum) : sum(sum), offset(0), ready(true) {};
      CheckSum* sum;
      unsigned long long int offset;
      bool ready;
    };
    
    std::list<checksum_desc> checksums;

  public:
    /// This object controls transfer speed
    DataSpeed speed;
    /// Check if DataBuffer object is initialized
    operator bool() const {
      return (bufs != 0);
    }
    /// Contructor
    /// \param size size of every buffer in bytes.
    /// \param blocks number of buffers.
    DataBuffer(unsigned int size = 65536, int blocks = 3);
    /// Contructor
    /// \param size size of every buffer in bytes.
    /// \param blocks number of buffers.
    /// \param cksum object which will compute checksum. Should not be
    /// destroyed till DataBuffer itself.
    DataBuffer(CheckSum *cksum, unsigned int size = 65536, int blocks = 3);
    /// Destructor.
    ~DataBuffer();
    /// Reinitialize buffers with different parameters.
    /// \param size size of every buffer in bytes.
    /// \param blocks number of buffers.
    /// \param cksum object which will compute checksum. Should not be
    /// destroyed till DataBuffer itself.
    bool set(CheckSum *cksum = NULL, unsigned int size = 65536,
             int blocks = 3);
    /// Add a checksum object which will compute checksum of buffer.
    /// \param cksum object which will compute checksum. Should not be
    /// destroyed till DataBuffer itself.
    /// \return integer position in the list of checksum objects.
    int add(CheckSum *cksum);
    /// Direct access to buffer by number.
    char* operator[](int n);
    /// Request buffer for READING INTO it.
    /// \param handle returns buffer's number.
    /// \param length returns size of buffer
    /// \param wait if true and there are no free buffers, method will wait
    /// for one.
    /// \return true on success
    bool for_read(int& handle, unsigned int& length, bool wait);
    /// Check if there are buffers which can be taken by for_read(). This
    /// function checks only for buffers and does not take eof and error
    /// conditions into account.
    bool for_read();
    /// Informs object that data was read into buffer.
    /// \param handle buffer's number.
    /// \param length amount of data.
    /// \param offset offset in stream, file, etc.
    bool is_read(int handle, unsigned int length,
                 unsigned long long int offset);
    /// Informs object that data was read into buffer.
    /// \param buf - address of buffer
    /// \param length amount of data.
    /// \param offset offset in stream, file, etc.
    bool is_read(char *buf, unsigned int length,
                 unsigned long long int offset);
    /// Request buffer for WRITING FROM it.
    /// \param handle returns buffer's number.
    /// \param length returns size of buffer
    /// \param wait if true and there are no free buffers, method will wait
    /// for one.
    bool for_write(int& handle, unsigned int& length,
                   unsigned long long int& offset, bool wait);
    /// Check if there are buffers which can be taken by for_write(). This
    /// function checks only for buffers and does not take eof and error
    /// conditions into account.
    bool for_write();
    /// Informs object that data was written from buffer.
    /// \param handle buffer's number.
    bool is_written(int handle);
    /// Informs object that data was written from buffer.
    /// \param buf - address of buffer
    bool is_written(char *buf);
    /// Informs object that data was NOT written from buffer (and releases
    /// buffer).
    /// \param handle buffer's number.
    bool is_notwritten(int handle);
    /// Informs object that data was NOT written from buffer (and releases
    /// buffer).
    /// \param buf - address of buffer
    bool is_notwritten(char *buf);
    /// Informs object if there will be no more request for 'read' buffers.
    /// v true if no more requests.
    void eof_read(bool v);
    /// Informs object if there will be no more request for 'write' buffers.
    /// v true if no more requests.
    void eof_write(bool v);
    /// Informs object if error accured on 'read' side.
    /// \param v true if error.
    void error_read(bool v);
    /// Informs object if error accured on 'write' side.
    /// \param v true if error.
    void error_write(bool v);
    /// Returns true if object was informed about end of transfer on 'read'
    /// side.
    bool eof_read();
    /// Returns true if object was informed about end of transfer on 'write'
    /// side.
    bool eof_write();
    /// Returns true if object was informed about error on 'read' side.
    bool error_read();
    /// Returns true if object was informed about error on 'write' side.
    bool error_write();
    /// Returns true if eror occured inside object.
    bool error_transfer();
    /// Returns true if object was informed about error or internal error
    /// occured.
    bool error();
    /// Wait (max 60 sec.) till any action happens in object.
    /// Returns true if action is eof on any side.
    bool wait_any();
    /// Wait till there are no more used buffers left in object.
    bool wait_used();
    /// Returns true if checksum was successfully computed, returns
    /// false if index is not in list.
    /// \param index of the checksum in question.
    bool checksum_valid() const;
    bool checksum_valid(int index) const;
    /// Returns CheckSum object specified in constructor, returns
    /// NULL if index is not in list.
    /// \param index of the checksum in question.
    const CheckSum* checksum_object() const;
    const CheckSum* checksum_object(int index) const;
    /// Wait till end of transfer happens on 'read' side.
    bool wait_eof_read();
    /// Wait till end of transfer or error happens on 'read' side.
    bool wait_read();
    /// Wait till end of transfer happens on 'write' side.
    bool wait_eof_write();
    /// Wait till end of transfer or error happens on 'write' side.
    bool wait_write();
    /// Wait till end of transfer happens on any side.
    bool wait_eof();
    /// Returns offset following last piece of data transfered.
    unsigned long long int eof_position() const {
      return eof_pos;
    }
    /// Returns size of buffer in object. If not initialized then this
    /// number represents size of default buffer.
    unsigned int buffer_size() const;
  };

} // namespace Arc

#endif // __ARC_DATABUFFER_H__
