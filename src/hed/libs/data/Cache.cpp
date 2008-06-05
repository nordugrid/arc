#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cerrno>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fcntl.h>

#include <arc/Logger.h>
#include <arc/data/Cache.h>

#ifdef WIN32
#include <arc/win32.h>
#endif

namespace Arc {

  static Logger logger(Logger::getRootLogger(), "Cache");

  static unsigned long long int cache_clean(const std::string& cache_path,
					    const std::string& cache_data_path,
					    const User& cache_user,
					    unsigned long long int size,
					    int h);

  // ------------------------------------------------------------------
  // General purpose functions
  // ------------------------------------------------------------------
  static ssize_t write_all(int h, const void *buf, size_t count) {
    size_t l = 0;
    ssize_t ll;
    for (; l < count;) {
      ll = write(h, ((char *)buf) + l, count - l);
      if (ll == -1)
	return (-1);
      l += ll;
    }
    return l;
  }

  // ------------------------------------------------------------------
  // General purpose list related functions
  // ------------------------------------------------------------------
#ifndef WIN32
  static int lock_file(int h) {
    int res;
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    for (;;) {
      if ((res = fcntl(h, F_SETLKW, &fl)) != 0)
	if (errno == EINTR)
	  continue;
      break;
    }
    return res;
  }

  static int unlock_file(int h) {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    return fcntl(h, F_SETLKW, &fl); // hope unlock should not fail
  }
#else
  static int lock_file(int h) {
    HANDLE hdl = (HANDLE)_get_osfhandle(h);
    DWORD low = 1, high = 0;
    if (hdl < 0)
      return -1;
    OVERLAPPED offset = {
      0, 0, 0, 0, NULL
    };
    UnlockFileEx(hdl, 0, low, high, &offset);
    return LockFileEx(hdl,
		      LOCKFILE_EXCLUSIVE_LOCK ? LOCKFILE_FAIL_IMMEDIATELY : 0,
		      0, low, high, &offset);
  }

  static int unlock_file(int h) {
    HANDLE hdl = (HANDLE)_get_osfhandle(h);
    DWORD low = 1, high = 0;
    if (hdl < 0)
      return -1;
    OVERLAPPED offset = {
      0, 0, 0, 0, NULL
    };
    return UnlockFileEx(hdl, 0, low, high, &offset);
  }
#endif

  // -1 - error, 0 - found, 1 - not found
  static int find_record(int h, const std::string& fname, off_t& record_start,
			 unsigned int& record_length,
			 bool match_full = false) {
    char buf[1024];
    int name_l = fname.length();
    int name_p = 0;
    bool before_name = true;
    bool skip_to_end = false;
    int l = 0;
    int i = 0;
    record_start = 0;
    record_length = 0;
    bool record_found = false;
    for (;;) {
      if (i >= l) {
	l = read(h, buf, sizeof(buf) - 1);
	if (l == -1)
	  return -1;
	if (l == 0) {
	  if (record_found)
	    break; // already have record
	  if (skip_to_end)
	    return 1;
	  if (!before_name)  // still comparing record
	    if (name_p >= name_l) {
	      record_found = true; // make it exit later
	      skip_to_end = true;
	      break;
	    }
	  return 1;
	}
	i = 0;
      }
      // process buffer
      if (before_name) { // look for name
	for (; i < l; i++)
	  if (buf[i] != 0)
	    break;
	if (i >= l)
	  continue; // read more
	before_name = false;
	name_p = 0;
	record_start = lseek(h, 0, SEEK_CUR) - l + i;
      }
      else {
	if (skip_to_end) {
	  for (; i < l; i++)
	    if (buf[i] == 0)
	      break;
	  if ((i >= l) && (l != 0))
	    continue; // read more
	  if (record_found)
	    break; // exit
	  before_name = true;
	  skip_to_end = false;
	  continue;
	}
	if (name_p < name_l)
	  for (; i < l;) {
	    if (fname[name_p] != buf[i]) {
	      skip_to_end = true;
	      break;
	    }
	    name_p++;
	    i++;
	    if (name_p >= name_l)
	      break;
	  }
	if (i >= l)
	  continue; // read more
	if (name_p >= name_l) {
	  if (((buf[i] == ' ') && (!match_full)) || (buf[i] == 0))  // found
	    record_found = true;
	  skip_to_end = true;
	}
      }
    }
    record_length = lseek(h, 0, SEEK_CUR) - l + i - record_start;
    lseek(h, record_start, SEEK_SET);
    return 0;
  }

  // -1 - failure, 0 - found
  static int find_empty_space(int h, int record_l) {
    lseek(h, 0, SEEK_SET);
    int found_l = 0;
    char buf[1024];
    for (;;) {
      int l = read(h, buf, sizeof(buf) - 1);
      if (l == -1)
	return -1;
      if (l == 0) { // end of file
	if (found_l > 0)
	  lseek(h, lseek(h, 0, SEEK_CUR) - found_l + 1, SEEK_SET);
	else {
	  char c = 0;
	  if (write(h, &c, 1) != 1)
	    return -1;
	}
	break;
      }
      int found_p = 0;
      for (; found_p < l; found_p++)
	if (buf[found_p] == 0) {
	  found_l++;
	  if (found_l > record_l)
	    break;
	}
	else
	  found_l = 0;
      if (found_l > record_l) {
	lseek(h, lseek(h, 0, SEEK_CUR) - l + found_p - record_l + 1, SEEK_SET);
	break;
      }
    }
    return 0;
  }

  // file pointer points at a begining of record
  static int cache_read_url_list(int h, std::string& url) {
    off_t o = lseek(h, 0, SEEK_CUR);
    char buf[256];
    url.resize(0);
    bool skip_filename = true;
    bool skip_spaces = true;
    for (;;) {
      int l = read(h, buf, sizeof(buf) - 1);
      if (l == -1) {
	lseek(h, o, SEEK_SET);
	return -1;
      }
      if (l == 0)
	break;
      buf[l] = 0;
      const char *p = buf;
      if (skip_filename) {
	for (; (*p != 0) && (*p != '\n'); p++)
	  if (*p == ' ') {
	    skip_filename = false;
	    break;
	  }
	if ((p - buf) == l)
	  continue;
	if (skip_filename) {
	  lseek(h, o, SEEK_SET);
	  return 0;
	}
      }
      if (skip_spaces) {
	for (; (*p != 0) && (*p != '\n'); p++)
	  if (*p != ' ') {
	    skip_spaces = false;
	    break;
	  }
	if ((p - buf) == l)
	  continue;
	if (skip_spaces) {
	  lseek(h, o, SEEK_SET);
	  return 0;
	}
      }
      url += p;
      l -= (p - buf);
      p = (const char *)memchr(p, 0, l);
      if (p)
	break;
    }
    std::string::size_type n = url.find('\n');
    if (n != std::string::npos)
      url.resize(n);
    lseek(h, o, SEEK_SET);
    return 0;
  }

  static int list_records(int h, std::list<std::string>& records) {
    char buf[1024];
    int l = 0; // amount of dat in buf
    int i = 0; // pointer in buf
    std::string record("");
    bool record_found = false;
    for (;;) {
      if (i >= l) {
	l = read(h, buf, sizeof(buf) - 1);
	if (l == -1)
	  return -1;
	if (l == 0) {
	  if (record_found)
	    records.push_back(record);
	  return 0;
	}
	i = 0;
	buf[l] = 0;
      }
      // process buffer
      if (!record_found) { // look for name
	for (; i < l; i++)
	  if (buf[i] != 0)
	    break;
	if (i >= l)
	  continue;
	record_found = true;
      }
      record += buf + i;
      for (; i < l; i++)
	if (buf[i] == 0)
	  break;
      if (i >= l)
	continue;
      records.push_back(record);
      record.resize(0);
      record_found = false;
    }
    return -1;
  }

  // ------------------------------------------------------------------
  // Functions to handle 'new' and 'old' lists
  // ------------------------------------------------------------------
  static int cache_history_add_record(const std::string& fname,
				      const std::string& name);
  static int cache_history_rem_record(const std::string& fname,
				      const std::string& name);

  static int cache_history_add_new(const std::string& cache_path,
				   const std::string& name) {
    std::string fname(cache_path);
    fname += "/new";
    int n = cache_history_add_record(fname, name);
    if (n != 0)
      return n;
    fname = cache_path;
    fname += "/old";
    return cache_history_rem_record(fname, name);
  }

  static int cache_history_add_old(const std::string& cache_path,
				   const std::string& name) {
    std::string fname(cache_path);
    fname += "/old";
    int n = cache_history_add_record(fname, name);
    if (n != 0)
      return n;
    fname = cache_path;
    fname += "/new";
    return cache_history_rem_record(fname, name);
  }

  static int cache_history_add_record(int h, const std::string& name) {
    int name_l = name.length() + 1;
    if (find_empty_space(h, name_l) == -1)
      return -1;
    if (write_all(h, name.c_str(), name_l) != name_l)
      return -1;
    return 0;
  }

  static int cache_history_add_record(const std::string& fname,
				      const std::string& name) {
    int h = open(fname.c_str(), O_RDWR);
    if (h == -1) {
      if (errno == ENOENT)
	return 0; // history disabled
      return -1;
    }
    if (lock_file(h) != 0) {
      close(h);
      return -1;
    }
    if (cache_history_add_record(h, name) != 0) {
      unlock_file(h);
      close(h);
      return -1;
    }
    unlock_file(h);
    close(h);
    return 0;
  }

  static int cache_history_rem_record(int h, const std::string& name) {
    off_t record_start;
    unsigned int record_length;
    lseek(h, 0, SEEK_SET);
    for (;;) {
      int n = find_record(h, name, record_start, record_length, true);
      if (n == -1)
	return -1; // failure
      if (n == 1)
	return 0; // not found
      unsigned char c = 0;
      for (; record_length; record_length--)
	if (write(h, &c, 1) != 1)
	  return -1; // failure
    }
  }

  static int cache_history_rem_record(const std::string& fname,
				      const std::string& name) {
    int h = open(fname.c_str(), O_RDWR);
    if (h == -1) {
      if (errno == ENOENT)
	return 0; // history disabled
      return -1;
    }
    if (lock_file(h) != 0) {
      close(h);
      return -1;
    }
    if (cache_history_rem_record(h, name) != 0) {
      unlock_file(h);
      close(h);
      return -1;
    }
    // failure
    unlock_file(h);
    close(h);
    return 0;
  }

  int cache_history_lists(const std::string& cache_path,
			  std::list<std::string>& olds,
			  std::list<std::string>& news) {
    std::string fname_old(cache_path);
    fname_old += "/old";
    std::string fname_new(cache_path);
    fname_new += "/new";
    int h_old = -1;
    int h_new = -1;

    h_old = open(fname_old.c_str(), O_RDWR);
    if (h_old == -1)
      if (errno != ENOENT)
	goto error_exit;
    if (h_old != -1)
      if (lock_file(h_old) != 0) {
	close(h_old);
	h_old = -1;
	goto error_exit;
      }

    h_new = open(fname_new.c_str(), O_RDWR);
    if (h_new == -1)
      if (errno != ENOENT)
	goto error_exit;
    if (h_new != -1)
      if (lock_file(h_new) != 0) {
	close(h_new);
	h_new = -1;
	goto error_exit;
      }

    if (h_old != -1)
      if (list_records(h_old, olds) != 0)
	goto error_exit;
    if (h_new != -1)
      if (list_records(h_new, news) != 0)
	goto error_exit;

    if (h_old != -1) {
      unlock_file(h_old);
      close(h_old);
    }
    if (h_new != -1) {
      unlock_file(h_new);
      close(h_new);
    }
    return 0;

 error_exit:
    if (h_old != -1) {
      unlock_file(h_old);
      close(h_old);
    }
    if (h_new != -1) {
      unlock_file(h_new);
      close(h_new);
    }
    return -1;
  }

  int cache_history_remove(const std::string& cache_path,
			   std::list<std::string>& olds,
			   std::list<std::string>& news) {
    std::string fname_old(cache_path);
    fname_old += "/old";
    std::string fname_new(cache_path);
    fname_new += "/new";
    int h_old = -1;
    int h_new = -1;

    h_old = open(fname_old.c_str(), O_RDWR);
    if (h_old == -1)
      if (errno != ENOENT)
	goto error_exit;
    if (h_old != -1)
      if (lock_file(h_old) != 0) {
	close(h_old);
	h_old = -1;
	goto error_exit;
      }

    h_new = open(fname_new.c_str(), O_RDWR);
    if (h_new == -1)
      if (errno != ENOENT)
	goto error_exit;
    if (h_new != -1)
      if (lock_file(h_new) != 0) {
	close(h_new);
	h_new = -1;
	goto error_exit;
      }

    if (h_old != -1)
      for (std::list<std::string>::iterator i = olds.begin(); i != olds.end();)
	if (cache_history_rem_record(h_old, *i) != 0)
	  ++i;
	else
	  i = olds.erase(i);
    if (h_new != -1)
      for (std::list<std::string>::iterator i = news.begin(); i != news.end();)
	if (cache_history_rem_record(h_new, *i) != 0)
	  ++i;
	else
	  i = news.erase(i);

    if (h_old != -1) {
      unlock_file(h_old);
      close(h_old);
    }
    if (h_new != -1) {
      unlock_file(h_new);
      close(h_new);
    }
    return 0;

 error_exit:
    if (h_old != -1) {
      unlock_file(h_old);
      close(h_old);
    }
    if (h_new != -1) {
      unlock_file(h_new);
      close(h_new);
    }
    return -1;
  }

  int cache_history(const std::string& cache_path, bool enable,
		    const User& cache_user) {
    std::string fname_old(cache_path);
    fname_old += "/old";
    std::string fname_new(cache_path);
    fname_new += "/new";
    int h_old = -1;
    int h_new = -1;
    if (enable) {
      h_old = open(fname_old.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      if (h_old == -1)
	goto error_exit;
      h_new = open(fname_new.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      if (h_new == -1)
	goto error_exit;
      int uid = cache_user.get_uid();
      if (uid)
	(chown(fname_old.c_str(), uid, cache_user.get_gid()) != 0);
      if (uid)
	(chown(fname_new.c_str(), uid, cache_user.get_gid()) != 0);
    }
    else {
      if (unlink(fname_old.c_str()) != 0)
	if (errno != ENOENT)
	  goto error_exit;
      if (unlink(fname_new.c_str()) != 0)
	if (errno != ENOENT)
	  goto error_exit;
    }
    if (h_old != -1)
      close(h_old);
    if (h_new != -1)
      close(h_new);
    return 0;
 error_exit:
    if (h_old != -1)
      close(h_old);
    if (h_new != -1)
      close(h_new);
    return -1;
  }

  // ------------------------------------------------------------------
  // Cache handling functions
  // ------------------------------------------------------------------

  static int cache_open_list(const std::string& cache_path,
			     const User& cache_user) {
    std::string fname(cache_path);
    fname += "/list";
    int h = open(fname.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (h == -1)
      return -1;
    int uid = cache_user.get_uid();
    if (uid)
      (chown(fname.c_str(), uid, cache_user.get_gid()) != 0);
    // lock file
    if (lock_file(h) != 0) {
      close(h);
      return -1;
    }
    return h;
  }

  static int cache_close_list(int h) {
    if (h == -1)
      return -1;
    unlock_file(h);
    close(h);
    return 0;
  }

  // returns: 0 - found, 1 - not found, -1 - error
  static int cache_read_list(int h, std::string& url, std::string& fname) {
    char c;
    for (;;) {
      url.erase();
      fname.erase();
      bool before_name = true;
      bool before_sep = true;
      bool before_url = true;
      int l = 0;
      for (;;) {
	l = read(h, &c, 1);
	if (l == -1)
	  return -1;
	if (l == 0)
	  break;
	if (before_name) {
	  if (c == 0)
	    continue;
	  before_name = false;
	}
	if (before_sep) {
	  if ((c != 0) && (c != ' ')) {
	    fname += c;
	    continue;
	  }
	  if (c == 0)
	    break;
	  before_sep = false;
	}
	if (before_url) {
	  if (c == ' ')
	    continue;
	  if (c == 0)
	    break;
	  before_url = false;
	}
	if (c != 0) {
	  url += c;
	  continue;
	}
	break;
      }
      if (fname.length() == 0) {
	if (l == 0)
	  return 1; // eof - no more names
	continue; // bad std::string - take next
      }
      break;
    }
    return 0;
  }

  // file_name url\0\0...\0file_name url\noptions...\0\0\0...\0...
  // returns: 0 - found, 1 - not found, -1 - error
  static int cache_search_list(int h, const std::string& url,
			       std::string& fname) {
    if (h == -1)
      return -1;
    lseek(h, 0, SEEK_SET);
    // look through file for given url
    char buf[1024];
    char name[256];
    bool before_name = true;
    bool before_sep = true;
    bool before_url = true;
    bool skip_to_end = false;
    int url_p = 0;
    int name_p = 0;
    int url_l = url.length();
    int l = 0;
    int i = 0;
    for (;;) {
      if (i >= l) {
	l = read(h, buf, sizeof(buf) - 1);
	if (l == -1)
	  return -1;
	if (l == 0)
	  return 1;
	i = 0;
      }
      // process buffer
      if (before_name) { // look for name
	for (; i < l; i++)
	  if (buf[i] != 0)
	    break;
	if (i >= l)
	  continue;
	before_name = false;
	before_sep = true;
	name_p = 0;
      }
      if (before_sep) {
	for (; i < l; i++) {
	  if ((buf[i] == ' ') || (buf[i] == 0))
	    break;
	  if (name_p < (sizeof(name) - 1)) {
	    name[name_p] = buf[i];
	    name_p++;
	  }
	}
	name[name_p] = 0;
	if (i >= l)
	  continue;
	if (buf[i] == 0) {
	  before_name = true;
	  continue;
	}
	before_sep = false;
	before_url = true;
      }
      if (before_url) {
	for (; i < l; i++)
	  if ((buf[i] != ' ') || (buf[i] == 0))
	    break;
	if (i >= l)
	  continue;
	if (buf[i] == 0) {
	  before_name = true;
	  continue;
	}
	before_url = false;
	skip_to_end = false;
	url_p = 0;
      }
      if (!skip_to_end) {
	if (url_p == url_l) { // end of url
	  if ((buf[i] == 0) || (buf[i] == '\n')) { // url found
	    fname = name;
	    // set pointer to begin of options
	    if (buf[i] == '\n')
	      i++;
	    lseek(h, lseek(h, 0, SEEK_CUR) - (l - i), SEEK_SET);
	    return 0;
	  }
	  skip_to_end = true;
	  continue;
	}
	int m_l = l - i;
	if (m_l > (url_l - url_p))
	  m_l = url_l - url_p;
	if (strncmp(url.c_str() + url_p, buf + i, m_l) != 0)
	  skip_to_end = true;
	else {
	  url_p += m_l;
	  i += m_l;
	}
      }
      if (skip_to_end) {
	for (; i < l; i++)
	  if (buf[i] == 0)
	    break;
	if (i >= l)
	  continue;
	before_name = true;
      }
    }
  }

  static char char_tab[16] = {
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'a', 'b',
    'c', 'd', 'e', 'f'
  };
  static void cache_file_name(int n, std::string& name) {
    for (int i = (sizeof(int) * 2 - 1); i >= 0; i--) {
      name[i] = char_tab[n & 0x0F];
      n >>= 4;
    }
    name[sizeof(int) * 2] = 0;
  }

  static int cache_open_info(const std::string& cache_path,
			     const std::string& fname) {
    std::string name = cache_path + '/' + fname + ".info";
    int h = open(name.c_str(), O_RDWR);
    if (h == -1)
      return -1;
    if (lock_file(h) != 0) {
      close(h);
      return -1;
    }
    return h;
  }

  // format:
  //  c/r/f/d+id
  //  created valid accessed
  // returns: -1 - error, 0 - success

  typedef struct {
    char st;
    std::string id;
  } cache_file_state;

  static int cache_write_info(int h, cache_file_state& fs) {
    lseek(h, 0, SEEK_SET);
    int l = write(h, &(fs.st), 1);
    if (l == -1)
      return -1;
    l = write(h, fs.id.c_str(), fs.id.length());
    if (l == -1)
      return -1;
    l = write(h, "\n", 1);
    if (l == -1)
      return -1;
    (ftruncate(h, lseek(h, 0, SEEK_CUR)) != 0);
    return 0;
  }

  static int cache_read_info(int h, cache_file_state& fs) {
    // read fist character - state of the file
    char state;
    lseek(h, 0, SEEK_SET);
    int l = read(h, &state, 1);
    if (l == -1)
      return -1;
    if (l == 0) { // empty file - assume 'c'
      fs.st = 'c';
      return 0;
    }
    fs.st = state;
    fs.id.erase();
    switch (state) {
    case 'c':
    case 'f':
      {
	// useless state
	return 0;
      }
      break;

    case 'd':
      {
	// useless yet - it is strange to read it
	// it should be id
	char c;
	for (;;) {
	  l = read(h, &c, 1);
	  if (l == -1)
	    return -1;
	  if (l == 0)
	    break;
	  if (c == '\n')
	    break;
	  fs.id += c;
	}
      }
      break;

    case 'r':
      {
	// very useful
      }
      break;

    default:
      return -1; // unknown state
    }
    return 0;
  }

  static int cache_close_info(int h) {
    unlock_file(h);
    return close(h);
  }

  // returns: 0 - added, -1 - error
  static int cache_add_list(int h, const std::string& url,
			    const std::string& cache_path,
			    const std::string& cache_data_path,
			    const User& cache_user,
			    const std::string& id, std::string& fname) {

#define OUT_OF_SPACE_CHECK						   \
    if (errno == ENOSPC) {  /* out of space - free some */		   \
      if (cache_clean(cache_path, cache_data_path, cache_user, 1, h) == 0) \
	i = INT_MAX; /* can't free anything */				   \
      i--;								   \
    }

    if (h == -1)
      return -1;
    std::string name;
    std::string name_info;
    std::string name_claim;
    std::string name_;
    int i = 0;
    for (; i < INT_MAX; i++) {
      cache_file_name(i, name_);
      name = cache_data_path + '/' + name_;
      // Although O_EXCL is broken on NFS, there is no race condition here
      // because list is locked
      int nh = open(name.c_str(), O_CREAT | O_EXCL | O_RDWR,
		    S_IRUSR | S_IWUSR);
      if (nh == -1) {
	OUT_OF_SPACE_CHECK;
	continue;
      }
      close(nh);
      name_info = cache_path + '/' + name_ + ".info";
      nh = open(name_info.c_str(), O_CREAT | O_EXCL | O_RDWR,
		S_IRUSR | S_IWUSR);
      if (nh == -1) {
	OUT_OF_SPACE_CHECK;
	remove(name.c_str());
	continue;
      }
      if (write_all(nh, "c\n", 2) == -1) {
	OUT_OF_SPACE_CHECK;
	close(nh);
	remove(name.c_str());
	remove(name_info.c_str());
	continue;
      }
      close(nh);
      name_claim = cache_path + '/' + name_ + ".claim";
      nh = open(name_claim.c_str(), O_CREAT | O_EXCL | O_RDWR,
		S_IRUSR | S_IWUSR);
      if (nh == -1) {
	OUT_OF_SPACE_CHECK;
	remove(name.c_str());
	remove(name_info.c_str());
	continue;
      }
      std::string jobclaim = id + "\n"; // claim file
      if (write_all(nh, jobclaim.c_str(), jobclaim.length()) == -1) {
	OUT_OF_SPACE_CHECK;
	close(nh);
	remove(name.c_str());
	remove(name_info.c_str());
	remove(name_claim.c_str());
	continue;
      }
      close(nh);
      if (cache_user.get_uid() != 0) {
	(chown(name.c_str(), cache_user.get_uid(), cache_user.get_gid()) != 0);
	(chown(name_info.c_str(), cache_user.get_uid(),
	       cache_user.get_gid()) != 0);
	(chown(name_claim.c_str(), cache_user.get_uid(),
	       cache_user.get_gid()) != 0);
      }
      else
	chmod(name.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      break;
    }
    if (i == INT_MAX)
      return -1;
    std::string record = name_ + " " + url;
    int record_l = record.length() + 1;
    // look through list file for enough empty space
    if (find_empty_space(h, record_l) == -1) {
      remove(name.c_str());
      remove(name_info.c_str());
      return -1;
    }
    off_t cur = lseek(h, 0, SEEK_CUR);
    int record_p = 0;
    for (; record_p < record_l;) {
      int l = write(h, record.c_str() + record_p, record_l - record_p);
      if (l == -1) {
	(ftruncate(h, cur) != 0);
	remove(name.c_str());
	return -1;
      }
      record_p += l;
    }
    cache_history_add_new(cache_path, url);
    fname = name_;
    return 0;
#undef OUT_OF_SPACE_CHECK
  }

  // returns: 0 - removed, -1 - error, 1 - not found
  // it does not check if that file is claimed - should be called ONLY for
  // non-claimed files
  static int cache_remove_list(int h, const std::string& fname,
			       const std::string& cache_path,
			       const std::string& cache_data_path,
			       const User&) {
    if (h == -1)
      return -1;
    std::string name = cache_data_path + '/' + fname;
    std::string name_info = cache_path + '/' + fname + ".info";
    std::string name_claim = cache_path + '/' + fname + ".claim";
    lseek(h, 0, SEEK_SET);
    // look through file for given name
    off_t record_start = 0;
    unsigned int record_length = 0;
    int record_res = find_record(h, fname, record_start, record_length);
    if (record_res == -1)
      return -1;
    if (record_res == 1)
      return 1;
    std::string url;
    if ((cache_read_url_list(h, url) == 0) && (url.length()))
      cache_history_add_old(cache_path, url);
    char c = 0;
    for (; record_length; record_length--)
      if (write_all(h, &c, 1) == -1)
	return -1;
    remove(name.c_str());
    remove(name_info.c_str());
    remove(name_claim.c_str());
    return 0;
  }

  static int cache_invalidate_list(int h, const std::string& cache_path,
				   const std::string&, const User&,
				   const std::string& fname) {
    if (h == -1)
      return -1;
    lseek(h, 0, SEEK_SET);
    // look through file for given name
    off_t record_start = 0;
    unsigned int record_length = 0;
    int record_res = find_record(h, fname, record_start, record_length);
    if (record_res == -1)
      return -1;
    if (record_res == 1)
      return 1;
    std::string url;
    if ((cache_read_url_list(h, url) == 0) && (url.length()))
      cache_history_add_old(cache_path, url);
    char c = 0;
    // invalidate record by removing it's id (url)
    int l = fname.length();
    lseek(h, l, SEEK_CUR);
    record_length -= l;
    for (; record_length; record_length--)
      if (write_all(h, &c, 1) == -1)
	return -1;
    int ih = cache_open_info(cache_path, fname);
    cache_file_state ist;
    ist.st = 'f'; // failure
    cache_write_info(ih, ist);
    cache_close_info(ih);
    return 0;
  }

  // replace id (url)
  static int cache_replace_list(int h, const std::string& fname,
				const std::string& url) {
    if (h == -1)
      return -1;
    lseek(h, 0, SEEK_SET);
    // look through file for given name
    off_t record_start = 0;
    unsigned int record_length = 0;
    int record_res = find_record(h, fname, record_start, record_length);
    if (record_res == -1)
      return -1;
    if (record_res == 1)
      return 1;
    // delete old record
    char c = 0;
    for (; record_length; record_length--)
      if (write_all(h, &c, 1) == -1)
	return -1;
    // create new record
    std::string record = fname + ' ' + url;
    int record_l = record.length() + 1;
    // find place for new
    if (find_empty_space(h, record_l) == -1)
      return -1;
    // write new record
    off_t cur = lseek(h, 0, SEEK_CUR);
    int record_p = 0;
    for (; record_p < record_l;) {
      int l = write(h, record.c_str() + record_p, record_l - record_p);
      if (l == -1) {
	(ftruncate(h, cur) != 0);
	return -1;
      }
      record_p += l;
    }
    return 0;
  }

  // release claimed cache file
  // returns: 0 - released, -1 - error
  static int cache_release_file(const std::string& cache_path,
				const std::string& id,
				const std::string& fname, bool once = false) {
    std::string name = cache_path + '/' + fname + ".claim";
    int h = open(name.c_str(), O_RDWR);
    if (h == -1)
      return -1;
    if (lock_file(h) != 0) {
      close(h);
      return -1;
    }
    // read whole file
    off_t flen = lseek(h, 0, SEEK_END);
    lseek(h, 0, SEEK_SET);
    char *fbuf = (char *)malloc(flen + 1);
    if (fbuf == NULL) {
      unlock_file(h);
      close(h);
      return -1;
    }
    fbuf[0] = 0;
    for (int p = 0; p < flen;) {
      int l = read(h, fbuf + p, flen - p);
      if (l == -1) {
	unlock_file(h);
	close(h);
	return -1;
      }
      if (l == 0) {
	flen = p;
	break;
      }
      p += l;
      fbuf[p] = 0;
    }
    // look for id
    int fbuf_p = 0;
    for (;;) {
      char *id_ = NULL;
      if (fbuf_p < flen)
	id_ = strstr(fbuf + fbuf_p, id.c_str());
      if (id_ != NULL) {
	int id_s = id_ - fbuf;
	int id_e = id_s + id.length();
	if (id_s != 0)
	  if ((fbuf[id_s - 1] != 0) && (fbuf[id_s - 1] != '\n')) {
	    fbuf_p = id_s + 1;
	    continue;
	  }
	if ((fbuf[id_e] != '\n') && (fbuf[id_e] != 0)) {
	  fbuf_p = id_s + 1;
	  continue;
	}
	// found
	id_e++;
	memmove(fbuf + id_s, fbuf + id_e, flen - id_e);
	flen -= (id_e - id_s);
	fbuf[flen] = 0;
	if (once)
	  break;
      }
      else  // not found any more
	break;
    }
    lseek(h, 0, SEEK_SET);
    ssize_t l = write(h, fbuf, flen);
    (ftruncate(h, flen) != 0);
    if (l != flen) {
      unlock_file(h);
      close(h);
      return 1;
    }
    unlock_file(h);
    close(h);
    return 0;
  }

  // returns: 0 - in use, 1 - is free
  int cache_is_claimed_file(const std::string& cache_path,
			    const std::string& fname) {
    std::string name = cache_path + '/' + fname + ".claim";
    struct stat st;
    if (stat(name.c_str(), &st) != 0)
      return 1; // probably missing == free
    if (st.st_size == 0)
      return 1;
    return 0;
  }

  static int cache_file_info(const std::string& cache_path,
			     const std::string& cache_data_path,
			     const std::string& fname, bool& claimed,
			     unsigned long long int& size, time_t& accessed) {
    struct stat st;
    std::string name = cache_path + '/' + fname + ".claim";
    if (stat(name.c_str(), &st) != 0)
      claimed = false;
    else if (st.st_size == 0)
      claimed = false;
    else
      claimed = true;
    name = cache_data_path + '/' + fname;
    if (stat(name.c_str(), &st) != 0)
      return -1;
    size = st.st_size;
    accessed = st.st_atime;
    return 0;
  }

  // claim already existing cache file
  static int cache_claim_file(const std::string& cache_path,
			      const std::string& id,
			      const std::string& fname) {
    std::string name = cache_path + '/' + fname + ".claim";
    int h = open(name.c_str(), O_RDWR);
    if (h == -1)
      return -1;
    if (lock_file(h) != 0) {
      close(h);
      return -1;
    }
    lseek(h, 0, SEEK_END);
    std::string jobclaim = id + "\n"; // claim file
    if (write(h, jobclaim.c_str(), jobclaim.length()) == -1) {
      unlock_file(h);
      close(h);
      return -1;
    }
    unlock_file(h);
    close(h);
    return 0;
  }

  int cache_claiming_list(const std::string& cache_path,
			  const std::string& fname,
			  std::list<std::string>& ids) {
    std::string name = cache_path + '/' + fname + ".claim";
    int h = open(name.c_str(), O_RDWR);
    if (h == -1)
      return -1;
    if (lock_file(h) != 0) {
      close(h);
      return -1;
    }
    // read whole file
    off_t flen = lseek(h, 0, SEEK_END);
    lseek(h, 0, SEEK_SET);
    char *fbuf = (char *)malloc(flen + 1);
    if (fbuf == NULL) {
      unlock_file(h);
      close(h);
      return -1;
    }
    fbuf[0] = 0;
    for (int p = 0; p < flen;) {
      int l = read(h, fbuf + p, flen - p);
      if (l == -1) {
	unlock_file(h);
	close(h);
	return -1;
      }
      if (l == 0) {
	flen = p;
	break;
      }
      p += l;
      fbuf[p] = 0;
    }
    unlock_file(h);
    close(h);
    int fbuf_p = 0;
    for (;;) {
      if (fbuf_p >= flen)
	break;
      char *id_ = fbuf + fbuf_p;
      for (; fbuf_p < flen; fbuf_p++) {
	if (fbuf[fbuf_p] == '\n')
	  break;
	if (fbuf[fbuf_p] == 0)
	  break;
      }
      fbuf[fbuf_p] = 0;
      std::string new_id(id_);
      for (std::list<std::string>::iterator i = ids.begin();
	   i != ids.end(); ++i)
	if (new_id == (*i)) {
	  new_id.resize(0);
	  break;
	}
      if (new_id.length() != 0)
	ids.push_back(new_id);
      fbuf_p++;
    }
    return 0;
  }

  // non-blocking fast check for file state
  static char cache_read_info_nonblock(const std::string& cache_path,
				       const std::string& fname) {
    std::string name = cache_path + '/' + fname + ".info";
    int h = open(name.c_str(), O_RDONLY);
    if (h == -1)
      return ' ';
    char state;
    lseek(h, 0, SEEK_SET);
    int l = read(h, &state, 1);
    if (l == -1) {
      close(h);
      return ' ';
    }
    if (l == 0) {
      close(h);
      return 'c';
    }
    close(h);
    return state;
  }

  // look for url in cache and attach to the file or create new
  // returns: 0 - success, 1 - unspecified error
  int cache_find_url(const std::string& cache_path,
		     const std::string& cache_data_path,
		     const User& cache_user, const std::string& url,
		     const std::string& id, std::string& options,
		     std::string& fname) {
    if (cache_path.empty())
      return 1;
    // open + lock list file
    int ch = cache_open_list(cache_path, cache_user);
    if (ch == -1)
      return 1;
    // std::string fname;
    switch (cache_search_list(ch, url, fname)) {
    case 0:
      {
	// url is found - claim it, check status and get options
	if (cache_claim_file(cache_path, id, fname) == -1) {
	  cache_close_list(ch);
	  return 1;
	}
	options = "";
	// get options
	char buf[256];
	int i, l;
	for (;;) {
	  l = read(ch, buf, sizeof(buf) - 1);
	  if (l == -1) {
	    cache_close_list(ch);
	    return 1;
	  }
	  if (l == 0)
	    break;
	  buf[l] = 0;
	  options += buf;
	  for (i = 0; i < l; i++)
	    if (buf[i] == 0)
	      break;
	  if (i < l)
	    break;
	}
      }
      break;

    case 1:
      {
	// no such url yet - create it, mark as 'created' and claim
	if (cache_add_list(ch, url, cache_path, cache_data_path, cache_user,
			   id, fname) == -1) {
	  cache_close_list(ch);
	  return 1;
	}
      }
      break;

    default:
      {
	cache_close_list(ch);
	return 1;
      }
    }
    // file is found or created and claimed
    cache_close_list(ch);
    return 0;
  }

  int cache_find_file(const std::string& cache_path, const std::string&,
		      const User& cache_user, const std::string& fname,
		      std::string& url, std::string& options) {
    if (cache_path.empty())
      return 1;
    // open + lock list file
    int ch = cache_open_list(cache_path, cache_user);
    if (ch == -1)
      return 1;
    // std::string fname;
    off_t record_start;
    unsigned int record_length;
    switch (find_record(ch, fname, record_start, record_length)) {
    case 0:
      {
	int l = fname.length() + 1;
	lseek(ch, l, SEEK_CUR);
	record_length -= l;
	options = "";
	// get options
	char buf[256];
	int i;
	for (;;) {
	  l = read(ch, buf, sizeof(buf) - 1);
	  if (l == -1) {
	    cache_close_list(ch);
	    return 1;
	  }
	  if (l == 0)
	    break;
	  buf[l] = 0;
	  options += buf;
	  for (i = 0; i < l; i++)
	    if (buf[i] == 0)
	      break;
	  if (i < l)
	    break;
	}
	// remove url
	url = options;
	std::string::size_type n = options.find('\n');
	if (n == std::string::npos)
	  options = "";
	else {
	  url.erase(n, std::string::npos);
	  options.erase(0, n + 1);
	}
      }
      break;

    default:
      cache_close_list(ch);
      return 1;
    }
    cache_close_list(ch);
    return 0;
  }

  int cache_release_url(const std::string& cache_path,
			const std::string& cache_data_path,
			const User& cache_user, const std::string& id,
			bool remove) {
    if (cache_path.empty())
      return 1;
    // open + lock list file
    int ch = cache_open_list(cache_path, cache_user);
    if (ch == -1)
      return 1;
    std::string fname;
    std::string url;
    bool fail = false;
    lseek(ch, 0, SEEK_SET);
    for (;;) {
      switch (cache_read_list(ch, url, fname)) {
      case 0:
	{
	  // have url and name
	  if (cache_release_file(cache_path, id, fname) == -1)
	    fail = true;
	  else if (remove) {
	    char state = cache_read_info_nonblock(cache_path, fname);
	    if (((state == 'f') || (state == 'c')) &&
		(cache_is_claimed_file(cache_path, fname) == 1))
	      if (cache_remove_list(ch, fname, cache_path, cache_data_path,
				    cache_user) != 0)
		fail = true;
	  }
	}
	break;

      case 1:
	{
	  cache_close_list(ch);
	  if (fail)
	    return 1;
	  return 0;
	}

      default:
	{
	  cache_close_list(ch);
	  return 1;
	}
      }
    }
  }

  int cache_release_url(const std::string& cache_path,
			const std::string& cache_data_path,
			const User& cache_user, const std::string& url,
			const std::string& id, bool remove) {
    if (cache_path.empty())
      return 1;
    // open + lock list file
    int ch = cache_open_list(cache_path, cache_user);
    if (ch == -1)
      return 1;
    std::string fname;
    switch (cache_search_list(ch, url, fname)) {
    case 0:
      {
	// url is found - release it
	// it is impossible to predict if that url was claimed more than once -
	// release completely
	if (cache_release_file(cache_path, id, fname) == -1) {
	  cache_close_list(ch);
	  return 1;
	}
	if (remove) {
	  char state = cache_read_info_nonblock(cache_path, fname);
	  if (((state == 'f') || (state == 'c')) &&
	      (cache_is_claimed_file(cache_path, fname) == 1))
	    if (cache_remove_list(ch, fname, cache_path, cache_data_path,
				  cache_user) != 0) {
	      cache_close_list(ch);
	      return 1;
	    }
	}
      }
      break;

    case 1:
      {
	// no such url - it died
	cache_close_list(ch);
	return 0;
      }

    default:
      cache_close_list(ch);
      return 1;
    }
    cache_close_list(ch);
    return 0;
  }

  int cache_release_file(const std::string& cache_path,
			 const std::string& cache_data_path,
			 const User& cache_user,
			 const std::string& fname, const std::string& id,
			 bool remove) {
    if (cache_path.empty())
      return 1;
    // open + lock list file
    int ch = cache_open_list(cache_path, cache_user);
    if (ch == -1)
      return 1;
    // claimed once - release once
    if (cache_release_file(cache_path, id, fname, true) == -1) {
      cache_close_list(ch);
      return 1;
    }
    if (remove) {
      char state = cache_read_info_nonblock(cache_path, fname);
      if (((state == 'f') || (state == 'c')) &&
	  (cache_is_claimed_file(cache_path, fname) == 1))
	if (cache_remove_list(ch, fname, cache_path, cache_data_path,
			      cache_user) != 0) {
	  cache_close_list(ch);
	  return 1;
	}
    }
    cache_close_list(ch);
    return 0;
  }

  int cache_invalidate_url(const std::string& cache_path,
			   const std::string& cache_data_path,
			   const User& cache_user,
			   const std::string& fname) {
    if (cache_path.empty())
      return 1;
    // open + lock list file
    int ch = cache_open_list(cache_path, cache_user);
    if (ch == -1)
      return 1;
    switch (cache_invalidate_list(ch, cache_path, cache_data_path, cache_user,
				  fname)) {
    case 0:
      {
	// url is invalidated
      }
      break;

    case 1:
      {
	// no such url - it died or was already invalidated
      }

    default:
      cache_close_list(ch);
      return 1;
    }
    cache_close_list(ch);
    return 0;
  }

  // before download (or waits) specified url into cache.
  // this can block for long
  // this function should be called with that file already claimed
  // returns: 0 - can download, 1 - error, 2 - file already exists
  // This function is unsafe because it can fail if url was invalidated
  int cache_download_url_start(const std::string& cache_path,
			       const std::string& cache_data_path,
			       const User& cache_user,
			       const std::string& url, const std::string& id,
			       cache_download_handler& handler) {
    if (cache_path.empty())
      return 1;
    if (handler.h != -1)
      return 0; // already locked
    // open + lock list file
    int ch = cache_open_list(cache_path, cache_user);
    if (ch == -1) {
      logger.msg(INFO, "cache_download_url_start: cache_open_list failed: %s",
		 cache_path);
      return 1;
    }
    std::string fname;
    switch (cache_search_list(ch, url, fname)) {
    case 0:
      {
	// url is found
	cache_close_list(ch); // do not need list anymore
      }
      break;

    case 1:
      {
	// no such url ?!?!?!?!?!?!
	logger.msg(INFO, "cache_download_url_start: url not found: %s", url);
	cache_close_list(ch);
	return 1;
      }

    default:
      {
	logger.msg(INFO, "cache_download_url_start: "
		   "unknown result from cache_search_list: %s", url);
	cache_close_list(ch);
	return 1;
      }
    }
    // try to get lock on info file
    logger.msg(INFO, "cache_download_url_start: locking url: %s (%s)",
	       url, fname);

    return cache_download_file_start(cache_path, cache_data_path, cache_user,
				     fname, id, handler);
  }

  int cache_download_file_start(const std::string& cache_path,
				const std::string& cache_data_path,
				const User& cache_user,
				const std::string& fname,
				const std::string& id,
				cache_download_handler& handler) {
    if (cache_path.empty())
      return 1;
    if (handler.h != -1)
      return 0; // already locked
    int ih = cache_open_info(cache_path, fname);
    if (ih == -1) {
      logger.msg(INFO, "cache_download_file_start: failed to lock file: %s",
		 fname);
      return 1;
    }
    logger.msg(INFO, "cache_download_file_start: locked file: %s", fname);
    handler.h = ih;
    handler.sname = fname;
    handler.fname = cache_data_path;
    handler.fname += "/";
    handler.fname += fname;
    // have exclusive lock on info file and name of the file to store data
    // read the state of the file
    cache_file_state fs;
    if (cache_read_info(ih, fs) == -1) {
      logger.msg(INFO, "cache_download_file_start: cache_read_info failed: %s",
		 fname);
      cache_close_info(ih);
      handler.h = -1;
      return 1;
    }
    switch (fs.st) {
    case 'd':
      {
	// it is probaly lock left by dead job
	logger.msg(INFO, "cache_download_file_start: state - dead: %s", fname);
	// remove all locks
	cache_release_url(cache_path, cache_data_path, cache_user, fs.id,
			  false);
      }

    case 'c':
    case 'f':
      {
	logger.msg(INFO, "cache_download_file_start: state - new/failed: %s",
		   fname);
	fs.st = 'd';
	fs.id = id;
	if (cache_write_info(ih, fs) == -1) {
	  cache_close_info(ih);
	  handler.h = -1;
	  return 1;
	}
      }
      return 0;

    case 'r':
      {
	logger.msg(INFO, "cache_download_file_start: state - ready: %s",
		   fname);
	cache_close_info(ih);
	handler.h = -1;
      }
      return 2;

    default:
      {
	logger.msg(INFO, "cache_download_file_start: state - UNKNOWN: %s",
		   fname);
	// behave like it would be new file
	fs.st = 'd';
	fs.id = id;
	if (cache_write_info(ih, fs) == -1) {
	  cache_close_info(ih);
	  handler.h = -1;
	  return 1;
	}
	cache_close_info(ih);
	handler.h = -1;
      }
      return 0;
    }
  }

  int cache_download_url_end(const std::string& cache_path, const std::string&,
			     const User& cache_user,
			     const std::string& url,
			     cache_download_handler& handler, bool success) {
    if (!url.empty()) {
      // open + lock list file
      int ch = cache_open_list(cache_path, cache_user);
      if (ch == -1)
	logger.msg(INFO, "cache_download_url_end: cache_open_list failed: %s",
		   cache_path);
      else {
	if (cache_replace_list(ch, handler.sname, url) != 0)
	  logger.msg(INFO, "cache_download_url_end: "
		     "file not found in list: %s", handler.sname);
	cache_close_list(ch);
      }
    }
    if (handler.h != -1) {
      cache_file_state fs;
      if (success)
	fs.st = 'r';
      else
	fs.st = 'f';
      fs.id.erase();
      if (cache_write_info(handler.h, fs) == -1) {
	cache_close_info(handler.h);
	handler.h = -1;
	return 1;
      }
      cache_close_info(handler.h);
      handler.h = -1;
    }
    return 0;
  }

  class cache_file_p {
  public:
    std::string name;
    unsigned long long int size;
    time_t accessed;
    bool valid;
    cache_file_p(const std::string& name_, unsigned long long int size_,
		 time_t accessed_, bool valid_)
      : name(name_),
	size(size_),
	accessed(accessed_),
	valid(valid_) {}
    bool operator<(const cache_file_p& right) {
      if (valid && (!right.valid))
	return false;
      if ((!valid) && right.valid)
	return true;
      return (accessed < right.accessed);
    }
  };

  // remove old files from cache to provide 'size' space
  unsigned long long int cache_clean(const std::string& cache_path,
				     const std::string& cache_data_path,
				     const User& cache_user,
				     unsigned long long int size) {
    return cache_clean(cache_path, cache_data_path, cache_user, size, -1);
  }

  static unsigned long long int cache_clean(const std::string& cache_path,
					    const std::string& cache_data_path,
					    const User& cache_user,
					    unsigned long long int size,
					    int h) {
    if (cache_path.empty())
      return 0;
    int ch;
    if (h == -1) {
      // open + lock list file
      ch = cache_open_list(cache_path, cache_user);
      if (ch == -1)
	return 0;
    }
    else {
      ch = h;
      lseek(h, 0, SEEK_SET);
    }
    std::list<cache_file_p> files;
    std::string fname;
    std::string url;
    lseek(ch, 0, SEEK_SET);
    bool finished = false;
    for (;;)
      if (finished)
	break;
    switch (cache_read_list(ch, url, fname)) {
    case 0:
      {
	// have name and (possibly) url
	// check if claimed
	bool claimed;
	unsigned long long int fsize = (unsigned long long int)-1;
	time_t accessed = -1;
	if (cache_file_info(cache_path, cache_data_path, fname, claimed,
			    fsize, accessed) != 0)
	  break;
	if (!claimed)
	  files.push_back(
	    cache_file_p(fname, fsize, accessed, url.length() != 0)
	    );
      }
      break;

    case 1:
      {
	// end of list
	finished = true;
      }
      break;

    default:
      {
	finished = true;
      }
      break;
    }
    files.sort();
    unsigned long long int total_size = 0;
    for (std::list<cache_file_p>::iterator i = files.begin();
	 i != files.end(); ++i) {
      logger.msg(INFO, "Removing cache file: name = %s, url = %s",
		 i->name, url);
      if (cache_remove_list(ch, i->name, cache_path, cache_data_path,
			    cache_user) == 0)
	total_size += i->size;
      if (!(i->valid))
	continue; // Remove all invalidated files
      if (total_size >= size)
	break;
    }
    logger.msg(INFO, "Cleaned %llu bytes in cache", total_size);
    if (h == -1)
      cache_close_list(ch);
    return total_size;
  }

  int cache_files_list(const std::string& cache_path, const User& cache_user,
		       std::list<std::string>& files) {
    if (cache_path.empty())
      return 0;
    // open + lock list file
    int ch = cache_open_list(cache_path, cache_user);
    if (ch == -1)
      return -1;
    std::string fname;
    std::string url;
    lseek(ch, 0, SEEK_SET);
    bool finished = false;
    for (;;)
      if (finished)
	break;
    switch (cache_read_list(ch, url, fname)) {
    case 0:
      {
	// have url and name
	files.push_back(fname);
      }
      break;

    case 1:
      {
	// end of list
	finished = true;
      }
      break;

    default:
      {
	finished = true;
      }
      break;
    }
    cache_close_list(ch);
    return 0;
  }

} // namespace Arc
