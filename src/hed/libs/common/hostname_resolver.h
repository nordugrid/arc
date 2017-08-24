#define CMD_PING (0)
// -

#define CMD_RESOLVE_TCP_LOCAL  (1)
#define CMD_RESOLVE_TCP_REMOTE (2)
// string hostname
// -
// result
// errno
// [
//  int family
//  socklen_t length
//  void addr[]
// ]*

