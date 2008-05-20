#ifndef __ARC_CACHE_H__
#define __ARC_CACHE_H__
/*
   Functions to support (relatively) low-level access to cache.
   Cache consists of plain files stored in directory refered as cache_path.
   It contains files named:
     list - stored names of the files (8 digit numbers) and corresponding
       urls delimited by blank space. Each pair is delimited by some
       amount of \0 codes.
     XXXXXXXX - files storing content of corresponding url.
     XXXXXXXX.info - stores status of files. Status is represented
       by one character:
   	c - just created, content is empty.
   	f - failed to download (treated same as 'c').
   	r - ready to use, content is valid.
   	d - beeing downloaded. 'd' is followed by identifier (id)
   	    of application downloading that file.
       During download of content this file has write lock set
       (short-term lock).
     XXXXXXXX.claim - stores list of applications' ids using this file.
       ids are stored one per line.
 */

#include <string>
#include <list>
#include <unistd.h>
#include <arc/User.h>

namespace Arc {

  class cache_download_handler;

  /*
     Functionality:
      Should be called before starting to download file to cache
      Checks for status of the file.
      If it is new or last download failed - returns immeadiately and
      application can start downloading.
      If it is beeing downloaded (locked) - waits till other application
      finishes.
     Accepts:
      cache_path - path to diretory containing cache.
      cache_user - user owning cache.
      id - string used to mark application/job claiming file.
      handler - object to hold short-term lock.
      url - source URL of file.
      fname - name of cache file.
     Returns:
      0 - application can download. short-term lock is set and
     	  cache_download_url_end should be called.
      1 - error, most probably cache corruption. short-term lock is not set.
      2 - file already existed or just been downloaded by another
     	  program. short-term lock is not set.
   */
  int cache_download_url_start(const std::string& cache_path,
			       const std::string& cache_data_path,
			       const Arc::User& cache_user,
			       const std::string& url,
			       const std::string& id,
			       cache_download_handler& handler);
  int cache_download_file_start(const std::string& cache_path,
				const std::string& cache_data_path,
				const Arc::User& cache_user,
				const std::string& fname,
				const std::string& id,
				cache_download_handler& handler);

  /*
     Functionality:
      Should be called after application finished downloading file to cache.
     Accepts:
      handler - object holding short-term lock.
      success - =true if file was successfuly downloaded, otherwise false.
     Returns:
      0 - success
      1 - error, means failed to write information about status of file.
     	  It is better to cancel in that case.
   */
  int cache_download_url_end(const std::string& cache_path,
			     const std::string& cache_data_path,
			     const Arc::User& cache_user,
			     const std::string& url,
			     cache_download_handler& handler, bool success);

  /*
     Object used to hold short-term lock of cached file during download.
     Also provides cache file name on file system.
   */
  class cache_download_handler {
    friend int cache_download_url_start(const std::string&,
					const std::string&,
					const Arc::User&,
					const std::string&,
					const std::string&,
					cache_download_handler&);
    friend int cache_download_url_end(const std::string& cache_path,
				      const std::string& cache_data_path,
				      const Arc::User& cache_user,
				      const std::string& url,
				      cache_download_handler& handler,
				      bool success);
    friend int cache_download_file_start(const std::string& cache_path,
					 const std::string& cache_data_path,
					 const Arc::User& cache_user,
					 const std::string& fname,
					 const std::string& id,
					 cache_download_handler& handler);
  private:
    int h;
    std::string sname;
    std::string fname;
  public:
    cache_download_handler(void)
      : h(-1),
	fname("") {}
    ~cache_download_handler(void) {
      if (h != -1)
	close(h);
    }
    const std::string& name(void) const {
      return fname;
    }
    const std::string& cache_name(void) const {
      return sname;
    }
  };

  /*
     Functionality:
      Look in cache for file with specified url. If not found, it will
      be created. File will be claimed using provided id.
     Accepts:
      cache_path - path to diretory containing cache.
      cache_user - user owning cache.
      url - url, which file is going to represent.
      id - string used to mark application/job claiming file.
     Returns:
      0 - success.
      1 - error.
   */
  int cache_find_url(const std::string& cache_path,
		     const std::string& cache_data_path,
		     const Arc::User& cache_user, const std::string& url,
		     const std::string& id, std::string& options,
		     std::string& fname);

  /*
     Functionality:
      Read url+options associated with given file
   */
  int cache_find_file(const std::string& cache_path,
		      const std::string& cache_data_path,
		      const Arc::User& cache_user, const std::string& fname,
		      std::string& url, std::string& options);

  /*
     Functionality:
      Unclaim file(s) claimed by id and (optionally) representing given url.
     Accepts:
      cache_path - path to diretory containing cache.
      cache_user - user owning cache.
      url - url, which file is going to represent.
      id - string used to mark application/job claiming file.
      remove - if file(s) is not properly downloaded and has no more claiming
     	       application, also remove it from cache physically.
     Returns:
      0 - success.
      1 - error.
   */
  int cache_release_url(const std::string& cache_path,
			const std::string& cache_data_path,
			const Arc::User& cache_user, const std::string& url,
			const std::string& id, bool remove);
  int cache_release_url(const std::string& cache_path,
			const std::string& cache_data_path,
			const Arc::User& cache_user, const std::string& id,
			bool remove);
  int cache_release_file(const std::string& cache_path,
			 const std::string& cache_data_path,
			 const Arc::User& cache_user,
			 const std::string& fname, const std::string& id,
			 bool remove);

  int cache_invalidate_url(const std::string& cache_path,
			   const std::string& cache_data_path,
			   const Arc::User& cache_user,
			   const std::string& fname);

  /*
     Functionality:
      Remove oldest files from cache.
     Accepts:
      cache_path - path to diretory containing cache.
      cache_user - user owning cache.
      size - amount of space to be freed.
     Returns:
      Amount of space freed.
   */
  unsigned long long int cache_clean(const std::string& cache_path,
				     const std::string& cache_data_path,
				     const Arc::User& cache_user,
				     unsigned long long int size);

  // These function are intended for internal usage.
  /*
     Functionality:
      Provides ids of applications currently claiming given file.
     Accepts:
      cache_path - path to directory containing cache.
      fname - name of the cache file (full path is cache_path+'/'+fname)
     Returns:
      ids - list of applications' ids.
      0 - success.
      -1 - error.
   */
  int cache_claiming_list(const std::string& cache_path,
			  const std::string& fname,
			  std::list<std::string>& ids);
  /*
     Functionality:
      checks if file is claimed
     Accepts:
      cache_path - path to directory containing cache.
      fname - name of the cache file (full path is cache_path+'/'+fname)
     Returns:
      1 - not claimed
      0 - claimed
      -1 - error
   */
  int cache_is_claimed_file(const std::string& cache_path,
			    const std::string& fname);
  /*
     Functionality:
      Provides names of files in cache.
     Accepts:
      cache_path - path to diretory containing cache.
      cache_user - user owning cache.
     Returns:
      files - list of all content files in cache.
      0 - success.
      -1 - error.
   */
  int cache_files_list(const std::string& cache_path,
		       const Arc::User& cache_user,
		       std::list<std::string>& files);

  // Obtain records stored in history files
  int cache_history_lists(const std::string& cache_path,
			  std::list<std::string>& olds,
			  std::list<std::string>& news);

  // Remove records from cache history
  int cache_history_remove(const std::string& cache_path,
			   std::list<std::string>& olds,
			   std::list<std::string>& news);

  // Enable/disable cache history. Disabling history deletes all records.
  int cache_history(const std::string& cache_path, bool enable,
		    const Arc::User& cache_user);

} // namespace Arc

#endif // __ARC_CACHE_H__
