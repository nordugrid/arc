#ifndef __ARC_NSSGETPASS_H__
#define __ARC_NSSGETPASS_H__

#include <nss.h>
#include <p12.h>

namespace AuthN {

  char* nss_get_password_from_console(PK11SlotInfo* slot, PRBool retry, void *arg);
}

#endif /*__ARC_NSSGETPASS_H__*/

