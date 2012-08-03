from distutils.core import setup

setup(name='arc-cacheindex-indexserver',
      version='svn',
      description='Cache index system for the ARC Middleware (indexserver)',
      author='Henrik Thostrup Jensen',
      author_email='htj@ndgf.org',
      url='http://www.nordugrid.org/',
      packages=['arc/cacheindex/indexserver'],

      data_files=[('/etc/init.d', ['datafiles/etc/acix-index']),
                  ('/etc',        ['datafiles/etc/acix-index.tac'])
      ]

)

