
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

#define CMD_HARDLINK (3)
// string oldname
// string newname
// -
// result
// errno

#define CMD_SOFTLINK (4)
// string oldname
// string newname
// -
// result
// errno

#define CMD_STAT (5)
// string path
// -
// result
// errno
// stat

#define CMD_LSTAT (6)
// string path
// -
// result
// errno
// stat

#define CMD_REMOVE (7)
// string path
// -
// result
// errno

#define CMD_UNLINK (8)
// string path
// -
// result
// errno

#define CMD_RMDIR (9)
// string path
// -
// result
// errno

#define CMD_OPENDIR (10)
// string path
// -
// result
// errno

#define CMD_CLOSEDIR (11)
// -
// result
// errno

#define CMD_READDIR (12)
// -
// result
// errno
// string name

#define CMD_OPENFILE (13)
// flags
// mode
// string path
// -
// result
// errno

#define CMD_CLOSEFILE (14)
// -
// result
// errno

#define CMD_READFILE (15)
// offset
// size
// -
// result
// errno
// string data

#define CMD_WRITEFILE (16)
// offset
// string data
// -
// result
// errno

#define CMD_SEEKFILE (17)
// offset
// whence
// -
// result
// errno
// offset

