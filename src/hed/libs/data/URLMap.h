// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_URLMAP_H__
#define __ARC_URLMAP_H__

#include <list>

#include <arc/URL.h>
#include <arc/Logger.h>

namespace Arc {

  /// URLMap allows mapping certain patterns of URLs to other URLs.
  /**
   * A URLMap can be used if certain URLs can be more efficiently accessed
   * by other means on a certain site. For example a GridFTP storage element
   * may be mounted as a local file system and so a map can be made from a
   * gsiftp:// URL to a local file path.
   * \ingroup data
   * \headerfile URLMap.h arc/data/URLMap.h
   */
  class URLMap {
  private:
    class map_entry {
    public:
      URL initial;
      URL replacement;
      URL access;
      map_entry() {}
      map_entry(const URL& templ, const URL& repl, const URL& accs = URL())
        : initial(templ),
          replacement(repl),
          access(accs) {}
    };
    std::list<map_entry> entries;
    static Logger logger;
  public:
    /// Construct an empty URLMap.
    URLMap();
    ~URLMap();
    /// Map a URL if possible.
    /**
     * If the given URL matches any template it will be changed to the mapped
     * URL. Additionally, if the mapped URL is a local file and existence_check
     * is true, a permission check is done by attempting to open the file. If a
     * different access path is specified for this URL the URL will be changed
     * to link://accesspath. If existence_check is false then the access path
     * is ignored. To check if a URL will be mapped without changing it local()
     * can be used.
     * \param url URL to check
     * \param existence_check If true, check if the mapped URL exists
     * \return true if the URL was mapped to a new URL, false if it was not
     * mapped or an error occurred during mapping
     */
    bool map(URL& url, bool existence_check=true) const;
    /// Check if a mapping exists for a URL.
    /**
     * Checks to see if a URL will be mapped but does not do the mapping.
     * @param url URL to check
     * @return true if a mapping exists for this URL
     */
    bool local(const URL& url) const;
    /// Add an entry to the URLMap.
    /**
     * All URLs matching templ will have the templ part replaced by repl.
     * @param templ template to replace, for example gsiftp://se.org/files
     * @param repl replacement for template, for example /export/grid/files
     * @param accs replacement path if it differs in the place the file will
     * actually be accessed (e.g. on worker nodes), for example
     * /mount/grid/files
     */
    void add(const URL& templ, const URL& repl, const URL& accs = URL());
    /// Returns true if the URLMap is not empty.
    operator bool() const { return entries.size() != 0; };
    /// Returns true if the URLMap is empty.
    bool operator!() const { return entries.size() == 0; };
  };

} // namespace Arc

#endif // __ARC_URLMAP_H__
