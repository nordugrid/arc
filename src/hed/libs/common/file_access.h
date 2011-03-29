
#define CMD_PING (0)
// -

#define CMD_SETUID (1)
// int uid
// int gid
// -
// result
// errno

#define CMD_MKDIR  (2)
// mode_t mode
// string dirname
// -
// result
// errno

#define CMD_MKDIRP  (3)
// mode_t mode
// string dirname
// -
// result
// errno

#define CMD_HARDLINK (4)
// string oldname
// string newname
// -
// result
// errno

#define CMD_SOFTLINK (5)
// string oldname
// string newname
// -
// result
// errno

#define CMD_COPY (6)
// mode_t mode
// string oldname
// string newname
// -
// result
// errno

#define CMD_STAT (7)
// string path
// -
// result
// errno
// stat

#define CMD_LSTAT (8)
// string path
// -
// result
// errno
// stat

#define CMD_REMOVE (9)
// string path
// -
// result
// errno

#define CMD_UNLINK (10)
// string path
// -
// result
// errno

#define CMD_RMDIR (11)
// string path
// -
// result
// errno

#define CMD_OPENDIR (12)
// string path
// -
// result
// errno

#define CMD_CLOSEDIR (13)
// -
// result
// errno

#define CMD_READDIR (14)
// -
// result
// errno
// string name

#define CMD_OPENFILE (15)
// flags
// mode
// string path
// -
// result
// errno

#define CMD_CLOSEFILE (16)
// -
// result
// errno

#define CMD_READFILE (17)
// size
// -
// result
// errno
// string data

#define CMD_WRITEFILE (18)
// string data
// -
// result
// errno

#define CMD_READFILEAT (19)
// offset
// size
// -
// result
// errno
// string data

#define CMD_WRITEFILEAT (20)
// offset
// string data
// -
// result
// errno

#define CMD_SEEKFILE (21)
// offset
// whence
// -
// result
// errno
// offset

#define CMD_FSTAT (22)
// -
// result
// errno
// stat

#define CMD_READLINK (23)
// string path
// -
// result
// errno
// stat
// string path

#define CMD_RMDIRR (24)
// string path
// -
// result
// errno

#define CMD_FTRUNCATE (25)
// length
// -
// result
// errno

#define CMD_FALLOCATE (26)
// length
// -
// result
// errno

