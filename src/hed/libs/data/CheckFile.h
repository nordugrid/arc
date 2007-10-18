namespace Arc {

int check_file_access(const char* path,int flags,uid_t uid,gid_t gid);
uid_t get_user_id(void);
gid_t get_user_group(uid_t uid);

} // namespace Arc
