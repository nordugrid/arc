#ifndef __ARC_USER_H__
#define __ARC_USER_H__
/** Platform independent represnetation of system user */
#include <string>

struct passwd;

namespace Arc
{

class User
{
    private:
        /* local name, home directory, uid and gid of this user */
        std::string name;
        std::string home;
        int uid;
        int gid;
        void set(struct passwd*);
    
    public:
        // get current user
        User();
        User(const std::string name);
        User(int uid);
        const std::string &Name(void) const { return name; };
        const std::string &Home(void) const { return home; };
        int get_uid(void) const { return (int)uid; };
        int get_gid(void) const { return (int)gid; };
        bool operator==(const std::string n) { return (n == name); };
        int check_file_access(const std::string &path, int flags);
        /* Run command as behalf of this user */ 
        bool RunAs(std::string cmd);
}; // class User

} // namespace Arc

#endif
