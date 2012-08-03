from distutils.core import setup

setup(name='arc-cacheindex-cacheserver',
      version='svn',
      description='Cache index system for the ARC Middleware (cacheserver)',
      author='Henrik Thostrup Jensen',
      author_email='htj@ndgf.org',
      url='http://www.nordugrid.org/',
      packages=['arc/cacheindex/cacheserver'],

      data_files=[('/etc/init.d/',          ['datafiles/etc/acix-cache']) ]

)

