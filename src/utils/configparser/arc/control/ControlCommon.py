import os
import logging

logger = logging.getLogger('ARCCTL.Common')

try:
    import ControlPaths
    ARC_LIBEXEC_DIR = ControlPaths.ARC_LIBEXEC_DIR
    ARC_DATA_DIR = ControlPaths.ARC_DATA_DIR
except ImportError:
    ControlPaths = None
    logger.error('There are no installation specific paths defined. '
                 'It seams you are running arcctl without autoconf - hardcoded defaults will be used.')

    ARC_LOCATION = '/usr'
    if 'ARC_LOCATION' in os.environ:
        ARC_LOCATION = os.environ['ARC_LOCATION']

    ARC_LIBEXEC_DIR = ARC_LOCATION + '/libexec/arc/'
    ARC_DATA_DIR = ARC_LOCATION + '/share/arc/'


class ComponentControl(object):
    """ Common abstract class to ensure all implicit calls to methods are defined """
    def control(self, args):
        raise NotImplementedError()

    @staticmethod
    def register_parser(root_parser):
        raise NotImplementedError()

