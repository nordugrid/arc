ARC Cache Index, core module
---------------------------------------

Required software:

  * Python. Only 2.5 and 2.6 have been tested.
  * Twisted Core and twisted web.
    On Ubuntu the packages are named: python-twisted-core and python-twisted-web
  * pyOpenSSL (package name python-openssl in Ubuntu)
  * (Python 2.5 only) Simple JSON, python-simplejson in Ubuntu


The core module doesn't include the cache or index service, but can be used as
a client module.


Installation instructions:

$ python setup.py install

It is possible to to specify an non-default installation location with --prefix, e.g.:
$ python setup.py install --prefix=/opt/arc-cacheindex/

However, it is typically --install-lib that you want to use, as prefix creates a rather
deep directory structure. Use:
$ python setup.py install --install-lib=/opt/arc-cacheindex/

Note that you will then need to set PYTHONPATH when relocating.

-- Index protocol --

To query an index service, construct a URL, like this:

https://orval.grid.aau.dk:6443/data/index?url=http://www.nordugrid.org:80/data/echo.sh

Here you ask the index services located at https://orval.grid.aau.dk:6443/data/index
for the location(s) of the file http://www.nordugrid.org:80/data/echo.sh

It is possible to query for multiple files by comma-seperating the files, e.g.:

index?url=http://www.nordugrid.org:80/data/echo.sh,http://www.nordugrid.org:80/data/echo.sh

Remember to quote/urlencode the strings when performing the get (wget and curl
will do this automatically, but most http libraries won't)

The result a JSON encoded datastructure with the top level structure being a
dictionary/hash-table with the mapping: url -> [machines], where [machines] is
a list of the machines of which the files is cached on. You should always use a
JSON parser to decode the result (the string might be escaped).

