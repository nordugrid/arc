#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <arc/Logger.h>
#include <arc/URL.h>

#include "RLS.h"

namespace Arc {

  static Logger logger(Logger::rootLogger, "RLS");

  bool rls_find_lrcs(const URL& url, rls_lrc_callback_t callback, void *arg) {
    std::list<URL> rlis;
    std::list<URL> lrcs;
    rlis.push_back(url);
    lrcs.push_back(url);
    return rls_find_lrcs(rlis, lrcs, true, true, callback, arg);
  }

  bool rls_find_lrcs(const URL& url, std::list<URL> lrcs) {
    std::list<URL> rlis;
    rlis.push_back(url);
    lrcs.clear();
    lrcs.push_back(url);
    return rls_find_lrcs(rlis, lrcs, true, true, NULL, NULL);
  }

  bool rls_find_lrcs(std::list<URL> rlis, std::list<URL> lrcs,
                     rls_lrc_callback_t callback, void *arg) {
    return rls_find_lrcs(rlis, lrcs, true, true, callback, arg);
  }

  bool rls_find_lrcs(std::list<URL> rlis, std::list<URL> lrcs, bool down,
		     bool up, rls_lrc_callback_t callback, void *arg) {
    globus_result_t err;
    int errcode;
    char errmsg[MAXERRMSG + 32];

    globus_rls_client_set_timeout(30);

    std::list<URL>::iterator lrc_p;
    std::list<URL>::iterator rli_p;
    globus_list_t *rliinfo_list;

    // Check predefined LRCs and call callback if LRC.
    int lrc_num = 0;
    for(lrc_p = lrcs.begin(); lrc_p != lrcs.end();) {
      const URL& url = *lrc_p;
      globus_rls_handle_t *h = NULL;
      logger.msg(INFO, "Contacting %s", url.str().c_str());
      err = globus_rls_client_connect((char*)url.ConnectionURL().c_str(), &h);
      if(err != GLOBUS_SUCCESS) {
	globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
				     GLOBUS_FALSE);
	logger.msg(INFO, "Warning: can't connect to RLS server %s: %s",
		   url.str().c_str(), errmsg);
	lrc_p = lrcs.erase(lrc_p);
	continue;
      }
      err = globus_rls_client_lrc_rli_list(h, &rliinfo_list);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
				     GLOBUS_FALSE);
        if(errcode == GLOBUS_RLS_INVSERVER) { // Not LRC
          globus_rls_client_close(h);
	  lrc_p = lrcs.erase(lrc_p);
	  continue;
        }
        else if(errcode == GLOBUS_RLS_RLI_NEXIST) { // Top level LRC server !?
          if(callback)
	    if(!callback(h, url, arg)) {
              globus_rls_client_close(h);
	      return false;
            }
          globus_rls_client_close(h);
	  ++lrc_p;
	  lrc_num++;
	  continue;
        }
        else {
          logger.msg(INFO,
		     "Warning: can't get list of RLIs from server %s: %s",
		     url.str().c_str(), errmsg);
          globus_rls_client_close(h);
	  lrc_p = lrcs.erase(lrc_p);
	  continue;
        }
      }
      else { // Add obtained RLIs to list
        if(up) {
	  for(globus_list_t *p = rliinfo_list; p; p = globus_list_rest(p)) {
	    URL url(((globus_rls_rli_info_t*)globus_list_first(p))->url);
	    if(std::find(rlis.begin(), rlis.end(), url) == rlis.end())
	      rlis.push_back(url);
	  }
	}
        globus_rls_client_free_list(rliinfo_list);
        if(callback)
	  if(!callback(h, url, arg)) {
            globus_rls_client_close(h);
	    return false;
          }
      }
      globus_rls_client_close(h);
      ++lrc_p;
      lrc_num++;
    }

    if(up) { // Go up through hierarchy looking for new RLIs and LRCs
      for(rli_p = rlis.begin(); rli_p != rlis.end();) {
        globus_rls_handle_t *h = NULL;
        const URL& url = *rli_p;
        logger.msg(INFO, "Contacting %s", url.str().c_str());
        err = globus_rls_client_connect((char*)url.ConnectionURL().c_str(),
					&h);
        if(err != GLOBUS_SUCCESS) {
	  globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
				       GLOBUS_FALSE);
	  logger.msg(INFO, "Warning: can't connect to RLS server %s: %s",
		     url.str().c_str(), errmsg);
	  rli_p = rlis.erase(rli_p);
	  continue;
        }
        bool bad_rli = false;
        err = globus_rls_client_rli_rli_list(h, &rliinfo_list);
        if(err != GLOBUS_SUCCESS) {
          globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
				       GLOBUS_FALSE);
          if(errcode == GLOBUS_RLS_INVSERVER) { // Not an RLI server
            bad_rli = true;
          }
          else if(errcode == GLOBUS_RLS_RLI_NEXIST) { // Top level server ?

          }
          else {
            logger.msg(INFO,
		       "Warning: can't get list of RLIs from server %s: %s",
		       url.str().c_str(), errmsg);
          }
        }
        else { // Add obtained RLIs to list
	  for(globus_list_t *p = rliinfo_list; p; p = globus_list_rest(p)) {
	    URL url(((globus_rls_rli_info_t*)globus_list_first(p))->url);
	    if(std::find(rlis.begin(), rlis.end(), url) == rlis.end())
	      rlis.push_back(url);
	  }
          globus_rls_client_free_list(rliinfo_list);
        }
        // Also check if this RLI is not LRC
        bool good_lrc = true;
        err = globus_rls_client_lrc_rli_list(h, &rliinfo_list);
        if(err != GLOBUS_SUCCESS) {
          globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
				       GLOBUS_FALSE);
          if(errcode == GLOBUS_RLS_INVSERVER) {
            good_lrc = false;
          }
          else if(errcode == GLOBUS_RLS_RLI_NEXIST) {
	    // Top level LRC server !?
          }
          else {
            good_lrc = false;
            logger.msg(INFO,
		       "Warning: can't get list of RLIs from server %s: %s",
		       url.str().c_str(), errmsg);
          }
        }
        else { // Add obtained RLIs to list
	  for(globus_list_t *p = rliinfo_list; p; p = globus_list_rest(p)) {
	    URL url(((globus_rls_rli_info_t*)globus_list_first(p))->url);
	    if(std::find(rlis.begin(), rlis.end(), url) == rlis.end())
	      rlis.push_back(url);
	  }
          globus_rls_client_free_list(rliinfo_list);
        }
        if(good_lrc) { // Add LRC to list
	  if(std::find(lrcs.begin(), lrcs.end(), url) == lrcs.end()) {
	    lrcs.push_back(url);
	    if(callback)
	      if(!callback(h, url, arg)) {
		globus_rls_client_close(h);
		return false;
	      }
	  }
	  lrc_num++;
        }
        globus_rls_client_close(h);
        if(bad_rli) {
          rli_p = rlis.erase(rli_p);
        }
        else {
          ++rli_p;
        }
      }
    }

    if(down) { // Go down through hierarchy looking for new RLIs and LRCs
      for(rli_p = rlis.begin(); rli_p != rlis.end();) {
        globus_rls_handle_t *h = NULL;
        const URL& url = *rli_p;
        logger.msg(INFO, "Contacting %s", url.str().c_str());
        err = globus_rls_client_connect((char*)url.ConnectionURL().c_str(),
					&h);
        if(err != GLOBUS_SUCCESS) {
	  globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
				       GLOBUS_FALSE);
	  logger.msg(INFO, "Warning: can't connect to RLS server %s: %s",
		     url.str().c_str(), errmsg);
	  rli_p = rlis.erase(rli_p);
	  continue;
        }
        globus_list_t *senderinfo_list;
        err = globus_rls_client_rli_sender_list(h, &senderinfo_list);
        if(err != GLOBUS_SUCCESS) {
          globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
				       GLOBUS_FALSE);
          if(errcode == GLOBUS_RLS_INVSERVER) { // Not an RLI server
            globus_rls_client_close(h);
            rli_p = rlis.erase(rli_p);
	    continue;
          }
          else if(errcode == GLOBUS_RLS_LRC_NEXIST) { // Empty RLI
            globus_rls_client_close(h);
            ++rli_p;
	    continue;
          }
          else {
            logger.msg(INFO,
		       "Warning: can't get list of senders from server %s: %s",
		       url.str().c_str(), errmsg);
            globus_rls_client_close(h);
            ++rli_p;
	    continue;
          }
        }
        else { // Add obtained LRCs to list
	  for(globus_list_t *p = senderinfo_list; p; p = globus_list_rest(p)) {
	    URL url(((globus_rls_sender_info_t*)globus_list_first(p))->url);
	    if(std::find(lrcs.begin(), lrcs.end(), url) == lrcs.end())
	      lrcs.push_back(url);
	  }
        }
        globus_rls_client_close(h);
        ++rli_p;
      }
    }

    // Go through new LRCs and check if those are LRCs
    for(lrc_p = lrcs.begin(); lrc_p != lrcs.end();) {
      if(lrc_num) {
        ++lrc_p;
	lrc_num--;
	continue;
      }
      const URL& url = *lrc_p;
      globus_rls_handle_t *h = NULL;
      logger.msg(INFO, "Contacting %s", url.str().c_str());
      err = globus_rls_client_connect((char*)url.ConnectionURL().c_str(), &h);
      if(err != GLOBUS_SUCCESS) {
	globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
				     GLOBUS_FALSE);
	logger.msg(INFO, "Warning: can't connect to RLS server %s: %s",
		   url.str().c_str(), errmsg);
	lrc_p = lrcs.erase(lrc_p);
	continue;
      }
      err = globus_rls_client_lrc_rli_list(h, &rliinfo_list);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
				     GLOBUS_FALSE);
        if(errcode == GLOBUS_RLS_INVSERVER) { // Not LRC
          globus_rls_client_close(h);
	  lrc_p = lrcs.erase(lrc_p);
	  continue;
        }
        else if(errcode == GLOBUS_RLS_RLI_NEXIST) { // Top level LRC server !?
          if(callback)
	    if(!callback(h, url, arg)) {
              globus_rls_client_close(h);
	      return false;
            }
          globus_rls_client_close(h);
	  ++lrc_p;
	  continue;
        }
        else {
          logger.msg(INFO,
		     "Warning: can't get list of RLIs from server %s: %s",
		     url.str().c_str(), errmsg);
          globus_rls_client_close(h);
	  lrc_p = lrcs.erase(lrc_p);
	  continue;
        }
      }
      else { // Call callback
        globus_rls_client_free_list(rliinfo_list);
        if(callback)
	  if(!callback(h, url, arg)) {
            globus_rls_client_close(h);
	    return false;
          }
      }
      globus_rls_client_close(h);
      ++lrc_p;
      lrc_num++;
    }
    return true;
  }

} // namespace Arc
