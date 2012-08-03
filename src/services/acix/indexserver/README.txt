ARC Cache Index, indexserver module
---------------------------------------

Required software:

  * Python. Only 2.5 and 2.6 have been tested.
  * Twisted Core and twisted web.
    On Ubuntu the packages are named: python-twisted-core and python-twisted-web
  * pyOpenSSL (package name python-openssl in Ubuntu)
  * (Python 2.5 only) Simple JSON, python-simplejson in Ubuntu


Installation instructions:

$ python setup.py installl

It is possible to to specify an non-default installation location with --prefix, e.g.:
$ python setup.py install --prefix=/opt/arc-cacheindex/

However, it is typically --install-lib that you want to use, as prefix creates a rather
deep directory structure. Use:
$ python setup.py install --install-lib=/opt/arc-cacheindex/

Note that you will then need to set PYTHONPATH when relocating.

And edit /etc/acix-cache.tac to include cache URLs.


Starting instructions.

$ /etc/init.d/acix-cache start

Update your rc* catalogs accordingly.

You can stop the daemon with:
$ /etc/init.d/acix-cache stop

By default the index server will listen on port 6443 (ssl+http) so you need to
open this port (or the configured port) in the firewall.

It is possible to configure port, use of ssl, and the index refresh interval.
See the indexsetup.py file (a bit of Python understanding is required).

