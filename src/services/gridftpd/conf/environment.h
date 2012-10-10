#ifndef __GRIDFTPD_ENVIRONMENT_H__
#define __GRIDFTPD_ENVIRONMENT_H__

#include <string>

namespace gridftpd {

  class GMEnvironment {
    bool valid_;
   public:
    GMEnvironment(bool guess = false);
    operator bool(void) const { return valid_; };
    bool operator!(void) const { return !valid_; };

    /// ARC configuration file
    ///   /etc/arc.conf
    ///   $ARC_LOCATION/etc/arc.conf
    std::string nordugrid_config_loc(void) const;
    void nordugrid_config_loc(const std::string&);

    // Certificates directory location
    std::string cert_dir_loc() const;

    /// Email address of person responsible for this ARC installation
    /// grid.manager@hostname, it can also be set from configuration file
    std::string support_mail_address(void) const;
    void support_mail_address(const std::string&);

  };

} // namespace gridftpd

#endif // __GRIDFTPD_ENVIRONMENT_H__
