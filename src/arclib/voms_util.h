#ifndef __ARC_VOMSUTIL_H__
#define __ARC_VOMSUTIL_H__

#include <arc/ArcConfig.h>
#include <arc/Logger.h>


namespace ArcLib {

/**class VOMSUtil is in charge of contacting the voms server and getting the attribute result*/
class VOMSUtil {
private:
        
public:
  contact(const std::string &hostname, int port, const std::string &contact,
	const std::string &command, std::string &buffer, std::string &username,
	std::string &ca);

}


}// namespace ArcLib

#endif /* __ARC_VOMSUTIL_H__ */

