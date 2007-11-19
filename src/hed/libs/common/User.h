#ifndef __ARC_USER_H__
#define __ARC_USER_H__
/** Platform independent represnetation of system user */
#include <string>

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
    
    public:
        User(std::string name);
        User(int uid);
        const std::string & Name(void) const { return name; };
        const std::string & Home(void) const { return home; };
        int get_uid(void) const { return (int)uid; };
        int get_gid(void) const { return (int)gid; };
        bool operator==(std::string n) { return (n == name); };
        /* Run command as behalf of this user */ 
        bool RunAs(std::string cmd);
}; // class User

}; // namespace Arc

#endif
