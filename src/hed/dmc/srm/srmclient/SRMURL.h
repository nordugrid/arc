#ifndef __HTTPSD_SRM_URL_H__
#define __HTTPSD_SRM_URL_H__

#include <arc/URL.h>

//namespace Arc {
  
  class SRMURL:public Arc::URL {
   private:
    static std::string empty;
    std::string filename;
    bool isshort;
    bool valid;
   public:
    /**
     * Examples shown for functions below assume the object was initiated with
     * srm://srm.ndgf.org/pnfs/ndgf.org/data/atlas/disk/user/user.mlassnig.dataset.1/dummyfile3
     */
    SRMURL(const char*);
    ~SRMURL(void) { };
  
    /**
     * eg /srm/managerv2
     */
    const std::string& Endpoint(void) const { return Path(); };
  
    /**
     * eg pnfs/ndgf.org/data/atlas/disk/user/user.mlassnig.dataset.1/dummyfile3
     */
    const std::string& FileName(void) const { if(!valid) return empty; return filename; };
  
    /**
     * eg httpg://srm.ndgf.org:8443/srm/managerv2
     */
    std::string ContactURL(void) const ;
  
    /**
     * eg srm://srm.ndgf.org:8443/srm/managerv2?SFN=
     */
    std::string BaseURL(void) const;
  
    /**
     * eg srm://srm.ndgf.org:8443/pnfs/ndgf.org/data/atlas/disk/user/user.mlassnig.dataset.1/dummyfile3
     */
    std::string ShortURL(void) const;
  
    /**
     * eg srm://srm.ndgf.org:8443/srm/managerv2?SFN=pnfs/ndgf.org/data/atlas/disk/user/user.mlassnig.dataset.1/dummyfile3
     */
    std::string FullURL(void) const;
  
    bool Short(void) const { return isshort; };
    bool GSSAPI(void) const;
    operator bool(void) { return valid; };
    bool operator!(void) { return !valid; };
  };
  
  /*
   * Subclass to override the URL pieces specific to the SRM version.
   */
  class SRM22URL: public SRMURL {
   private:
    static std::string empty;
    std::string filename;
    bool isshort;
    bool valid;
   public:
    SRM22URL(const char*);
  };

//} // namespace Arc

#endif // __HTTPSD_SRM_URL_H__
