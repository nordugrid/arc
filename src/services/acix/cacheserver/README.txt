ARC Cache Index, cacheserver module.
---------------------------------------

Required software:

  * Python. Only 2.5 and 2.6 have been tested.
  * Twisted Core and twisted web.
  * pyOpenSSL

Package names:

Debian / Ubuntu : python-twisted-core and python-twisted-web python-openssl

RedHat / CensOS : python-twisted-core python-twisted-web pyOpenSSL


Installation instructions:

$ python setup.py install.

It is possible to to specify an non-default installation location with --prefix, e.g.:
$ python setup.py install --prefix=/opt/arc-cacheindex/

However, it is typically --install-lib that you want to use, as prefix creates a rather
deep directory structure. Use:
$ python setup.py install --install-lib=/opt/arc-cacheindex/

You will then need to set PYTHONPATH. The init.d script acix-cache will be
installed in /etc/init.d/ - even when --prefix is used. If you don't like this
you can use --root (though some file shuffling will probably be necessary).



Configuration instructions:

Usually no configuration is necessary.

It is possible to specify custom logfile location by setting the logfile
parameter in arc.conf, like this:

---
[cacheserver]
logfile="/tmp/arc-cacheserver.log"
---

Note for old-style cache config:

If you have an old style cache config with cachedir and cachedata entries, you
need to specify the cache dirs manually. This can be done either in the .tac
file (which is generated in the init.d script) or directly in the code
(arc/cacheindex/cacheserver/setup.py).



Starting instructions:

/etc/init.d/acix-cache start

Update your rc* catalogs accordingly.

You can stop the daemon with:
$ /etc/init.d/acix-cache stop

You can inspect the log file to check that everything is running. It is located
at /var/log/acix-cache.log. An initial warning about the creation of zombie
process is typically generated (no zombie processes from the program has been
observed). If any zombie processes are observed, please let htj know.


Send the URL at which your cache filter is located at, to the index admins(s).
Unless you changed anything in the configuration, this will be:
https://HOST_FQDN:5443/data/cache

This is important as the index server pull the cache filter, from your site
(the filter doesn't get registered automatically).

It is possible to configure port, use of ssl, and the cache refresh interval.
See the cachesetup.py file (a bit of Python understanding is required).
Typically this is not needed.

By default, the cache server will listen on port 5443 (ssl+http), so you need
to open this (or the configured port) in your firewall.

