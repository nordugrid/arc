import os
import logging

logger = logging.getLogger('ARCCTL.Common')

# Define system paths
arc_location = '/usr'
if 'ARC_LOCATION' in os.environ:
    arc_location = os.environ['ARC_LOCATION']

ARC_LIBEXEC_DIR = arc_location + '/libexec/arc/'
ARC_DATA_DIR = arc_location + '/share/arc/'

try:
    import ControlPaths
    ARC_LIBEXEC_DIR = ControlPaths.libexecdir
    ARC_DATA_DIR = ControlPaths.datadir
except ImportError:
    logger.error('There are no installation specific paths defined. '
                 'It seams you are running arcctl without autoconf - hardcoded defaults will be used.')


class ComponentControl(object):
    """ Common abstract class to ensure all implicit calls to methods are defined """
    def control(self, args):
        raise NotImplementedError()

    @staticmethod
    def register_parser(root_parser):
        raise NotImplementedError()

