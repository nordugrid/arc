from distutils.core import setup

setup(name='arc-cacheindex-core',
      version='svn',
      description='Cache index system for the ARC Middleware (core)',
      author='Henrik Thostrup Jensen',
      author_email='htj@ndgf.org',
      url='http://www.nordugrid.org/',
      packages=['arc', 'arc/cacheindex', 'arc/cacheindex/core'],
)

