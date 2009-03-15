// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>

#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <arc/data/MkDirRecursive.h>

#ifdef WIN32
#include <arc/win32.h>
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

static int mkdir_force(const char *path, mode_t mode);

int mkdir_recursive(const std::string& base_path, const std::string& path,
                    mode_t mode, const Arc::User& user) {
  std::string name = base_path;
#ifndef WIN32
  if (path[0] != '/')
    name += '/';
#else
#endif
  name += path;
  std::string::size_type name_start = base_path.length();
  std::string::size_type name_end = name.length();
  /* go down */
  for (;;) {
    if ((mkdir_force(name.substr(0, name_end).c_str(), mode) == 0) ||
        (errno == EEXIST)) {
      if (errno != EEXIST)
        (lchown(name.substr(0, name_end).c_str(), user.get_uid(),
                user.get_gid()) != 0);
      /* go up */
      for (;;) {
        if (name_end >= name.length())
          return 0;
        name_end = name.find(DIR_SEPARATOR, name_end + 1);
        if (mkdir(name.substr(0, name_end).c_str(), mode) != 0) {
          if (errno == EEXIST)
            continue;
          return -1;
        }
        chmod(name.substr(0, name_end).c_str(), mode);
        (lchown(name.substr(0, name_end).c_str(), user.get_uid(),
                user.get_gid()) != 0);
      }
    }
    /* if(errno == EEXIST) { free(name); errno=EEXIST; return -1; } */
    if ((name_end = name.rfind(DIR_SEPARATOR, name_end - 1)) ==
        std::string::npos)
      break;
    if (name_end == name_start)
      break;
  }
  return -1;
}

int mkdir_force(const char *path, mode_t mode) {
  struct stat st;
  int r;
  if (stat(path, &st) != 0) {
    r = mkdir(path, mode);
    if (r == 0)
      (void)chmod(path, mode);
    return r;
  }
  if (S_ISDIR(st.st_mode)) { /* simulate error */
    r = mkdir(path, mode);
    if (r == 0)
      (void)chmod(path, mode);
    return r;
  }
  if (remove(path) != 0)
    return -1;
  r = mkdir(path, mode);
  if (r == 0)
    (void)chmod(path, mode);
  return r;
}
