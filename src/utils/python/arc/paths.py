from __future__ import absolute_import

import os

try:
    # try to import -generated file
    from .paths_dist import *
except ImportError:
    # use defaults
    if 'ARC_LOCATION' in os.environ:
        ARC_LOCATION = os.environ['ARC_LOCATION']
    else:
        ARC_LOCATION = '/usr'
    
    ARC_LIBEXEC_DIR = ARC_LOCATION + '/libexec/arc'
    ARC_DATA_DIR = ARC_LOCATION + '/share/arc'
    ARC_LIB_DIR = ARC_LOCATION + '/lib64/arc'
    ARC_RUN_DIR = '/var/run/arc'
    ARC_DOC_DIR = ARC_LOCATION + '/share/doc/nordugrid-arc/'
    ARC_CONF = '/etc/arc.conf'
    ARC_VERSION = 'devel'

# define ARC_LOCATION to be use by tools like gm-jobs
os.environ['ARC_LOCATION'] = ARC_LOCATION

