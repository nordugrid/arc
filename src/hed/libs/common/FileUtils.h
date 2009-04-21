#include <sys/types.h>
#include <unistd.h>
#include <glibmm.h>

namespace Arc {

int FileOpen(const char* path,int flags,mode_t mode);
int FileOpen(const char* path,int flags,uid_t uid,gid_t gid,mode_t mode);
Glib::Dir* DirOpen(const char* path);
Glib::Dir* DirOpen(const char* path,uid_t uid,gid_t gid);
bool FileStat(const char* path,struct stat *st,bool follow_symlinks);
bool FileStat(const char* path,struct stat *st,uid_t uid,gid_t gid,bool follow_symlinks);

} // namespace Arc

