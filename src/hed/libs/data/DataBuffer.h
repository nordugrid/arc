// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATABUFFER_H__
#define __ARC_DATABUFFER_H__

#include <arc/Thread.h>
#include <arc/data/DataSpeed.h>

namespace Arc {

  class CheckSum;

  /// Represents set of buffers.
  /**
   * This class is used during data transfer using DataPoint classes.
   * \ingroup data
   * \headerfile DataBuffer.h arc/data/DataBuffer.h
   */
  class DataBuffer {
  private:
    /// used to check if configuration changed
    int set_counter;
    /// general purpose mutex and condition used to achieve thread safety
    Glib::Mutex lock;
    Glib::Cond cond;
    /// internal struct to describe status of every buffer
    typedef struct {
      /// buffer address in memory
      char *start;
      /// true if taken by application for filling
      bool taken_for_read;
      /// true if taken by application for emptying
      bool taken_for_write;
      /// size of buffer
      unsigned int size;
      /// amount of information stored
      unsigned int used;
      /// offset in file or similar, has meaning only for application
      unsigned long long int offset;
    } buf_desc;
    /// amount of data passed through buffer (including current stored).
    /// computed using offset and size. gaps are ignored.
    unsigned long long int eof_pos;
    /// list of controlled buffers
    buf_desc *bufs;
    /// amount of controlled buffers
    int bufs_n;
    /// set to true if application's reading(filling) part won't use buffer anymore
    bool eof_read_flag;
    /// same for writing(emptying) part
    bool eof_write_flag;
    /// reading part of application experienced error
    bool error_read_flag;
    /// same for writing part
    bool error_write_flag;
    /// error was originated in DataBuffer itself
    bool error_transfer_flag;
    /// wait for any change of buffers' status
    bool cond_wait();
    /// internal class with pointer to object to compute checksum
    class checksum_desc {
     public:
      checksum_desc(CheckSum *sum)
        : sum(sum),
          offset(0),
          ready(true) {}
      CheckSum *sum;
      unsigned long long int offset;
      bool ready;
    };
    /// checksums to be computed in this buffer
    std::list<checksum_desc> checksums;
    /// Flag for DataPoint to claim buffer for exclusive use
    bool excl;

  public:
    /// This object controls transfer speed
    DataSpeed speed;
    /// Returns true if DataBuffer object is initialized
    operator bool() const {
      return (bufs != 0);
    }
    /// Construct a new DataBuffer object
    /**
     * \param size size of every buffer in bytes.
     * \param blocks number of buffers.
     */
    DataBuffer(unsigned int size = 1048576, int blocks = 3);
    /// Construct a new DataBuffer object with checksum computation
    /**
     * \param size size of every buffer in bytes.
     * \param blocks number of buffers.
     * \param cksum object which will compute checksum. Should not be
     * destroyed until DataBuffer itself.
     */
    DataBuffer(CheckSum *cksum, unsigned int size = 1048576, int blocks = 3);
    /// Destructor.
    ~DataBuffer();
    /// Reinitialize buffers with different parameters.
    /**
     * \param size size of every buffer in bytes.
     * \param blocks number of buffers.
     * \param cksum object which will compute checksum. Should not be
     * destroyed until DataBuffer itself.
     * \return true if buffers were successfully initialized
     */
    bool set(CheckSum *cksum = NULL, unsigned int size = 1048576,
             int blocks = 3);
    /// Add a checksum object which will compute checksum of buffer.
    /**
     * \param cksum object which will compute checksum. Should not be
     * destroyed until DataBuffer itself.
     * \return integer position in the list of checksum objects.
     */
    int add(CheckSum *cksum);
    /// Direct access to buffer by number.
    /**
     * \param n buffer number
     * \return buffer content
     */
    char* operator[](int n);
    /// Request buffer for READING INTO it.
    /**
     * Should be called when data is being read from a source. The calling code
     * should write data into the returned buffer and then call is_read().
     * \param handle filled with buffer's number.
     * \param length filled with size of buffer
     * \param wait if true and there are no free buffers, method will wait
     * for one.
     * \return true on success
     * For python bindings pattern of this method is
     * (bool, handle, length) for_read(wait). Here buffer for reading
     * to be provided by external code and provided to DataBuffer
     * object through is_read() method. Content of buffer must not exceed
     * provided length.
     */
    bool for_read(int& handle, unsigned int& length, bool wait);
    /// Check if there are buffers which can be taken by for_read().
    /**
     * This function checks only for buffers and does not take eof and error
     * conditions into account.
     * \return true if buffers are available
     */
    bool for_read();
    /// Informs object that data was read into buffer.
    /**
     * \param handle buffer's number.
     * \param length amount of data.
     * \param offset offset in stream, file, etc.
     * \return true if buffer was successfully informed
     * For python bindings pattern of that method is
     * bool is_read(handle,buffer,offset). Here buffer is string containing
     * content of buffer to be passed to DataBuffer object.
     */
    bool is_read(int handle, unsigned int length,
                 unsigned long long int offset);
    /// Informs object that data was read into buffer.
    /**
     * \param buf address of buffer
     * \param length amount of data.
     * \param offset offset in stream, file, etc.
     * \return true if buffer was successfully informed
     */
    bool is_read(char *buf, unsigned int length,
                 unsigned long long int offset);
    /// Request buffer for WRITING FROM it.
    /**
     * Should be called when data is being written to a destination. The
     * calling code should write the data contained in the returned buffer and
     * then call is_written().
     * \param handle returns buffer's number.
     * \param length returns size of buffer
     * \param offset returns buffer offset
     * \param wait if true and there are no available buffers,
     *     method will wait for one.
     * \return true on success
     * For python bindings pattern of this method is
     * (bool, handle, length, offset, buffer) for_write(wait).
     * Here buffer is string with content of buffer provided
     * by DataBuffer object.
     */
    bool for_write(int& handle, unsigned int& length,
                   unsigned long long int& offset, bool wait);
    /// Check if there are buffers which can be taken by for_write().
    /**
     * This function checks only for buffers and does not take eof and error
     * conditions into account.
     * \return true if buffers are available
     */
    bool for_write();
    /// Informs object that data was written from buffer.
    /**
     * \param handle buffer's number.
     * \return true if buffer was successfully informed
     */
    bool is_written(int handle);
    /// Informs object that data was written from buffer.
    /**
     * \param buf - address of buffer
     * \return true if buffer was successfully informed
     */
    bool is_written(char *buf);
    /// Informs object that data was NOT written from buffer (and releases buffer).
    /**
     * \param handle buffer's number.
     * \return true if buffer was successfully informed
     */
    bool is_notwritten(int handle);
    /// Informs object that data was NOT written from buffer (and releases buffer).
    /**
     * \param buf - address of buffer
     * \return true if buffer was successfully informed
     */
    bool is_notwritten(char *buf);
    /// Informs object if there will be no more request for 'read' buffers.
    /**
     * \param v true if no more requests.
     */
    void eof_read(bool v);
    /// Informs object if there will be no more request for 'write' buffers.
    /**
     * \param v true if no more requests.
     */
    void eof_write(bool v);
    /// Informs object if error occurred on 'read' side.
    /**
     * \param v true if error
     */
    void error_read(bool v);
    /// Informs object if error occurred on 'write' side.
    /**
     * \param v true if error
     */
    void error_write(bool v);
    /// Returns true if object was informed about end of transfer on 'read' side.
    bool eof_read();
    /// Returns true if object was informed about end of transfer on 'write' side.
    bool eof_write();
    /// Returns true if object was informed about error on 'read' side.
    bool error_read();
    /// Returns true if object was informed about error on 'write' side.
    bool error_write();
    /// Returns true if transfer was slower than limits set in speed object.
    bool error_transfer();
    /// Returns true if object was informed about error or internal error occurred.
    bool error();
    /// Wait (max 60 sec.) till any action happens in object.
    /**
     * \return true if action is eof on any side
     */
    bool wait_any();
    /// Wait till there are no more used buffers left in object.
    /**
     * \return true if an error occurred while waiting
     */
    bool wait_used();
    /// Wait till no more buffers taken for "READING INTO" left in object.
    /**
     * \return true if an error occurred while waiting
     */
    bool wait_for_read();
    /// Wait till no more buffers taken for "WRITING FROM" left in object.
    /**
     * \return true if an error occurred while waiting
     */
    bool wait_for_write();
    /// Returns true if the specified checksum was successfully computed.
    /**
     * \param index of the checksum in question.
     * \return false if index is not in list
     */
    bool checksum_valid(int index) const;
    /// Returns true if the checksum was successfully computed.
    bool checksum_valid() const;
    /// Returns CheckSum object at specified index or NULL if index is not in list.
    /**
     * \param index of the checksum in question.
     */
    const CheckSum* checksum_object(int index) const;
    /// Returns first checksum object in checksum list or NULL if list is empty.
    const CheckSum* checksum_object() const;
    /// Wait until end of transfer happens on 'read' side. Always returns true.
    bool wait_eof_read();
    /// Wait until end of transfer or error happens on 'read' side. Always returns true.
    bool wait_read();
    /// Wait until end of transfer happens on 'write' side. Always returns true.
    bool wait_eof_write();
    /// Wait until end of transfer or error happens on 'write' side. Always returns true.
    bool wait_write();
    /// Wait until end of transfer happens on any side. Always returns true.
    bool wait_eof();
    /// Returns offset following last piece of data transferred.
    unsigned long long int eof_position() const {
      return eof_pos;
    }
    /// Returns size of buffer in object.
    /**
     * If not initialized then this number represents size of default buffer.
     */
    unsigned int buffer_size() const;
    /// Informs that one DataPoint has taken the buffer for exclusive use
    void set_excl(bool v);
    /// Returns true if one DataPoint has taken the buffer for exclusive use
    bool get_excl() const;
  };

} // namespace Arc

#endif // __ARC_DATABUFFER_H__
